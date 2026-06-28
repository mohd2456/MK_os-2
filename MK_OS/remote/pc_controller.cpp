// ============================================================
// MK OS - Remote PC Controller
// Sends commands from Mac (brain) to a PC agent over TCP.
// Protocol: JSON lines over raw TCP sockets.
// ============================================================
#ifndef MK_PC_CONTROLLER_CPP
#define MK_PC_CONTROLLER_CPP

#include <iostream>
#include <string>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <atomic>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

class MKPCController {
private:
    int sockFd;
    std::string remoteIP;
    int remotePort;
    std::string authToken;
    std::atomic<bool> connected;
    int nextRequestId;
    std::string connectedHostname;  // Hostname of the connected PC agent

    // ─── JSON Helpers ────────────────────────────────────────────────────────

    std::string jsonEscape(const std::string& s) {
        std::string out;
        out.reserve(s.size() + 16);
        for (char c : s) {
            switch (c) {
                case '"':  out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\n': out += "\\n";  break;
                case '\r': out += "\\r";  break;
                case '\t': out += "\\t";  break;
                default:   out += c;      break;
            }
        }
        return out;
    }

    std::string jsonGetString(const std::string& json, const std::string& key) {
        std::string search = "\"" + key + "\"";
        size_t pos = json.find(search);
        if (pos == std::string::npos) return "";
        pos += search.size();
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == ':')) pos++;
        if (pos >= json.size()) return "";

        if (json[pos] == '"') {
            pos++;
            std::string value;
            while (pos < json.size()) {
                if (json[pos] == '\\' && pos + 1 < json.size()) {
                    pos++;
                    if (json[pos] == '"') value += '"';
                    else if (json[pos] == 'n') value += '\n';
                    else if (json[pos] == 't') value += '\t';
                    else if (json[pos] == '\\') value += '\\';
                    else value += json[pos];
                } else if (json[pos] == '"') {
                    break;
                } else {
                    value += json[pos];
                }
                pos++;
            }
            return value;
        }
        // Non-string value
        std::string value;
        while (pos < json.size() && json[pos] != ',' && json[pos] != '}') {
            value += json[pos];
            pos++;
        }
        while (!value.empty() && (value.back() == ' ' || value.back() == '\n'))
            value.pop_back();
        return value;
    }

    bool jsonGetBool(const std::string& json, const std::string& key) {
        std::string val = jsonGetString(json, key);
        return val == "true";
    }

    // ─── Socket Helpers ──────────────────────────────────────────────────────

    bool setSocketTimeout(int fd, int seconds) {
        struct timeval tv;
        tv.tv_sec = seconds;
        tv.tv_usec = 0;
        if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) return false;
        if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) return false;
        return true;
    }

    bool connectWithTimeout(int fd, struct sockaddr_in& addr, int timeoutSec) {
        // Set non-blocking
        int flags = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);

        int ret = ::connect(fd, (struct sockaddr*)&addr, sizeof(addr));
        if (ret == 0) {
            fcntl(fd, F_SETFL, flags); // restore blocking
            return true;
        }
        if (errno != EINPROGRESS) return false;

        // Wait for connection with poll
        struct pollfd pfd;
        pfd.fd = fd;
        pfd.events = POLLOUT;
        ret = poll(&pfd, 1, timeoutSec * 1000);

        if (ret <= 0) return false;

        // Check if connection succeeded
        int error = 0;
        socklen_t len = sizeof(error);
        getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len);

        // Restore blocking mode
        fcntl(fd, F_SETFL, flags);
        return error == 0;
    }

    std::string sendRaw(const std::string& data) {
        if (!connected.load() || sockFd < 0) return "";

        std::string msg = data + "\n";
        ssize_t sent = ::send(sockFd, msg.c_str(), msg.size(), 0);
        if (sent <= 0) {
            connected = false;
            return "";
        }

        // Read response line (up to 1MB)
        std::string response;
        response.reserve(4096);
        char buf[4096];
        while (true) {
            ssize_t n = ::recv(sockFd, buf, sizeof(buf) - 1, 0);
            if (n <= 0) {
                if (n == 0) connected = false;
                break;
            }
            buf[n] = '\0';
            response.append(buf, n);
            // Check for newline (end of JSON line)
            if (response.find('\n') != std::string::npos) break;
            if (response.size() > 1048576) break; // 1MB safety limit
        }
        // Trim trailing newline
        while (!response.empty() && (response.back() == '\n' || response.back() == '\r'))
            response.pop_back();
        return response;
    }

public:
    MKPCController()
        : sockFd(-1), connected(false), nextRequestId(1)
    {
        // Load config from environment
        const char* ip = std::getenv("MK_PC_IP");
        remoteIP = ip ? ip : "192.168.1.100";

        const char* port = std::getenv("MK_PC_PORT");
        remotePort = port ? std::atoi(port) : 9876;

        const char* token = std::getenv("MK_PC_TOKEN");
        authToken = token ? token : "";

        std::cout << "  [PC-CTRL] Remote PC controller initialized ("
                  << remoteIP << ":" << remotePort << ")\n";
    }

    ~MKPCController() {
        disconnect();
    }

    // ─── Discovery Types ─────────────────────────────────────────────────────

    struct DiscoveredAgent {
        std::string ip;
        int port;
        std::string hostname;
        std::string tokenHint;
    };

    // ─── Network Discovery ───────────────────────────────────────────────────

    std::vector<DiscoveredAgent> discoverAgents(int timeoutSeconds = 3) {
        std::vector<DiscoveredAgent> agents;

        int udpSock = socket(AF_INET, SOCK_DGRAM, 0);
        if (udpSock < 0) return agents;

        // Allow address reuse
        int reuse = 1;
        setsockopt(udpSock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

        // Bind to discovery port
        struct sockaddr_in bindAddr;
        std::memset(&bindAddr, 0, sizeof(bindAddr));
        bindAddr.sin_family = AF_INET;
        bindAddr.sin_port = htons(9877);
        bindAddr.sin_addr.s_addr = INADDR_ANY;

        if (bind(udpSock, (struct sockaddr*)&bindAddr, sizeof(bindAddr)) < 0) {
            ::close(udpSock);
            return agents;
        }

        // Set receive timeout
        struct timeval tv;
        tv.tv_sec = timeoutSeconds;
        tv.tv_usec = 0;
        setsockopt(udpSock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        // Listen for beacons until timeout
        auto startTime = std::chrono::steady_clock::now();
        while (true) {
            auto elapsed = std::chrono::steady_clock::now() - startTime;
            if (std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() >= timeoutSeconds)
                break;

            char buf[512];
            struct sockaddr_in fromAddr;
            socklen_t fromLen = sizeof(fromAddr);
            ssize_t n = recvfrom(udpSock, buf, sizeof(buf) - 1, 0,
                                 (struct sockaddr*)&fromAddr, &fromLen);
            if (n > 0) {
                buf[n] = '\0';
                std::string packet(buf);
                std::string senderIP = inet_ntoa(fromAddr.sin_addr);

                // Parse JSON beacon: {"service":"mk_agent","port":9876,"hostname":"...","token_hint":"..."}
                std::string service = jsonGetString(packet, "service");
                if (service == "mk_agent") {
                    DiscoveredAgent agent;
                    agent.ip = senderIP;
                    std::string portStr = jsonGetString(packet, "port");
                    agent.port = portStr.empty() ? 9876 : std::atoi(portStr.c_str());
                    agent.hostname = jsonGetString(packet, "hostname");
                    agent.tokenHint = jsonGetString(packet, "token_hint");

                    // Avoid duplicates
                    bool duplicate = false;
                    for (const auto& existing : agents) {
                        if (existing.ip == agent.ip && existing.port == agent.port) {
                            duplicate = true;
                            break;
                        }
                    }
                    if (!duplicate) {
                        agents.push_back(agent);
                    }
                }
            }
        }

        ::close(udpSock);
        return agents;
    }

    bool autoConnect() {
        auto agents = discoverAgents(3);
        if (agents.empty()) return false;

        for (const auto& agent : agents) {
            // Try connecting with stored auth token
            if (connect(agent.ip, agent.port, authToken)) {
                connectedHostname = agent.hostname;
                return true;
            }
        }
        return false;
    }

    std::string getConnectedHostname() const { return connectedHostname; }

    // ─── Connection Management ───────────────────────────────────────────────

    bool connect() {
        return connect(remoteIP, remotePort, authToken);
    }

    bool connect(const std::string& ip, int port, const std::string& token) {
        if (connected.load()) disconnect();

        remoteIP = ip;
        remotePort = port;
        authToken = token;

        sockFd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockFd < 0) {
            std::cerr << "  [PC-CTRL] Failed to create socket.\n";
            return false;
        }

        struct sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);

        if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
            std::cerr << "  [PC-CTRL] Invalid IP address: " << ip << "\n";
            ::close(sockFd);
            sockFd = -1;
            return false;
        }

        // Connect with 5-second timeout
        if (!connectWithTimeout(sockFd, addr, 5)) {
            ::close(sockFd);
            sockFd = -1;
            return false;
        }

        // Set 30-second timeout for send/recv
        setSocketTimeout(sockFd, 30);
        connected = true;

        // Verify connection with a ping
        std::string resp = sendCommand("ping", "");
        if (!jsonGetBool(resp, "success")) {
            disconnect();
            return false;
        }

        return true;
    }

    void disconnect() {
        if (sockFd >= 0) {
            ::close(sockFd);
            sockFd = -1;
        }
        connected = false;
    }

    bool isConnected() const {
        return connected.load();
    }

    std::string getIP() const { return remoteIP; }
    int getPort() const { return remotePort; }

    // ─── Command Interface ───────────────────────────────────────────────────

    std::string sendCommand(const std::string& type, const std::string& payload) {
        int id = nextRequestId++;
        std::string request = "{\"id\":" + std::to_string(id)
            + ",\"type\":\"" + jsonEscape(type)
            + "\",\"payload\":\"" + jsonEscape(payload)
            + "\",\"token\":\"" + jsonEscape(authToken) + "\"}";
        return sendRaw(request);
    }

    std::string sendCommandWithField(const std::string& type,
                                     const std::string& fieldName,
                                     const std::string& fieldValue) {
        int id = nextRequestId++;
        std::string request = "{\"id\":" + std::to_string(id)
            + ",\"type\":\"" + jsonEscape(type)
            + "\",\"" + fieldName + "\":\"" + jsonEscape(fieldValue)
            + "\",\"token\":\"" + jsonEscape(authToken) + "\"}";
        return sendRaw(request);
    }

    std::string sendCommandTwoFields(const std::string& type,
                                     const std::string& f1, const std::string& v1,
                                     const std::string& f2, const std::string& v2) {
        int id = nextRequestId++;
        std::string request = "{\"id\":" + std::to_string(id)
            + ",\"type\":\"" + jsonEscape(type)
            + "\",\"" + f1 + "\":\"" + jsonEscape(v1)
            + "\",\"" + f2 + "\":\"" + jsonEscape(v2)
            + "\",\"token\":\"" + jsonEscape(authToken) + "\"}";
        return sendRaw(request);
    }

    // ─── High-Level Commands ─────────────────────────────────────────────────

    std::string runShell(const std::string& command) {
        std::string resp = sendCommand("shell", command);
        if (resp.empty()) return "[PC-CTRL] No response (PC unreachable)";
        if (!jsonGetBool(resp, "success")) {
            std::string err = jsonGetString(resp, "stderr");
            return err.empty() ? "[PC-CTRL] Command failed" : err;
        }
        std::string out = jsonGetString(resp, "stdout");
        std::string err = jsonGetString(resp, "stderr");
        if (!err.empty() && !out.empty()) return out + "\n" + err;
        return out.empty() ? err : out;
    }

    std::string readFile(const std::string& path) {
        std::string resp = sendCommand("read_file", path);
        if (resp.empty()) return "[PC-CTRL] No response";
        if (!jsonGetBool(resp, "success")) {
            return "[PC-CTRL] Error: " + jsonGetString(resp, "error");
        }
        return jsonGetString(resp, "content");
    }

    std::string writeFile(const std::string& path, const std::string& content) {
        std::string resp = sendCommandTwoFields("write_file", "path", path, "content", content);
        if (resp.empty()) return "[PC-CTRL] No response";
        if (!jsonGetBool(resp, "success")) {
            return "[PC-CTRL] Error: " + jsonGetString(resp, "error");
        }
        return "File written successfully.";
    }

    std::string openApp(const std::string& appName) {
        std::string resp = sendCommand("open_app", appName);
        if (resp.empty()) return "[PC-CTRL] No response";
        if (!jsonGetBool(resp, "success")) {
            return "[PC-CTRL] Error: " + jsonGetString(resp, "error");
        }
        return "Application launched: " + appName;
    }

    std::string getSystemInfo() {
        std::string resp = sendCommand("system_info", "");
        if (resp.empty()) return "[PC-CTRL] No response";
        if (!jsonGetBool(resp, "success")) {
            return "[PC-CTRL] Error: " + jsonGetString(resp, "error");
        }
        std::string os = jsonGetString(resp, "os");
        std::string cpu = jsonGetString(resp, "cpu");
        std::string ram = jsonGetString(resp, "ram");
        std::string disk = jsonGetString(resp, "disk");

        std::string info;
        if (!os.empty())   info += "OS: " + os + "\n";
        if (!cpu.empty())  info += "CPU: " + cpu + "\n";
        if (!ram.empty())  info += "RAM: " + ram + "\n";
        if (!disk.empty()) info += "Disk: " + disk + "\n";
        return info.empty() ? jsonGetString(resp, "info") : info;
    }

    std::string listProcesses() {
        std::string resp = sendCommand("list_processes", "");
        if (resp.empty()) return "[PC-CTRL] No response";
        if (!jsonGetBool(resp, "success")) {
            return "[PC-CTRL] Error: " + jsonGetString(resp, "error");
        }
        return jsonGetString(resp, "output");
    }

    std::string killProcess(const std::string& nameOrPid) {
        std::string resp = sendCommand("kill_process", nameOrPid);
        if (resp.empty()) return "[PC-CTRL] No response";
        if (!jsonGetBool(resp, "success")) {
            return "[PC-CTRL] Error: " + jsonGetString(resp, "error");
        }
        return "Process killed: " + nameOrPid;
    }

    std::string getClipboard() {
        std::string resp = sendCommand("clipboard_get", "");
        if (resp.empty()) return "[PC-CTRL] No response";
        if (!jsonGetBool(resp, "success")) {
            return "[PC-CTRL] Error: " + jsonGetString(resp, "error");
        }
        return jsonGetString(resp, "content");
    }

    std::string setClipboard(const std::string& text) {
        std::string resp = sendCommand("clipboard_set", text);
        if (resp.empty()) return "[PC-CTRL] No response";
        if (!jsonGetBool(resp, "success")) {
            return "[PC-CTRL] Error: " + jsonGetString(resp, "error");
        }
        return "Clipboard updated.";
    }

    std::string screenshot() {
        std::string resp = sendCommand("screenshot", "");
        if (resp.empty()) return "[PC-CTRL] No response";
        if (!jsonGetBool(resp, "success")) {
            return "[PC-CTRL] Error: " + jsonGetString(resp, "error");
        }
        std::string path = jsonGetString(resp, "path");
        return path.empty() ? "Screenshot taken." : "Screenshot saved: " + path;
    }

    std::string listFiles(const std::string& directory) {
        std::string resp = sendCommand("list_files", directory);
        if (resp.empty()) return "[PC-CTRL] No response";
        if (!jsonGetBool(resp, "success")) {
            return "[PC-CTRL] Error: " + jsonGetString(resp, "error");
        }
        return jsonGetString(resp, "output");
    }
};

#endif // MK_PC_CONTROLLER_CPP
