// ============================================================
// MK OS - Query Server
// TCP server that accepts queries from remote clients (Windows CLI/Chat)
// Listens on port 9878 and routes queries through MK's brain.
// Thread-safe: uses mutex around MK brain access.
// ============================================================
#ifndef MK_QUERY_SERVER_CPP
#define MK_QUERY_SERVER_CPP

#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <functional>
#include <cstring>
#include <sstream>

// Platform-specific includes
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef int socklen_t;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif

// ============================================================
// MKQueryServer
// ============================================================

class MKQueryServer {
public:
    using QueryHandler = std::function<std::string(const std::string&)>;

private:
    int port_;
    std::string authToken_;
    std::atomic<bool> running_{false};
    std::thread listenThread_;
    SOCKET serverSocket_{INVALID_SOCKET};
    QueryHandler queryHandler_;
    std::mutex* brainMutex_;  // External mutex for brain access
    std::vector<std::thread> clientThreads_;
    std::mutex clientsMutex_;

    // Simple JSON value extraction
    static std::string jsonGetString(const std::string& json, const std::string& key) {
        std::string search = "\"" + key + "\"";
        size_t pos = json.find(search);
        if (pos == std::string::npos) return "";
        pos = json.find(':', pos + search.size());
        if (pos == std::string::npos) return "";
        pos = json.find('"', pos + 1);
        if (pos == std::string::npos) return "";
        pos++; // skip opening quote
        size_t end = pos;
        while (end < json.size() && json[end] != '"') {
            if (json[end] == '\\') end++; // skip escaped char
            end++;
        }
        return json.substr(pos, end - pos);
    }

    void handleClient(SOCKET clientSock, struct sockaddr_in clientAddr) {
        char addrStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, addrStr, sizeof(addrStr));
        std::cout << "  " << "\033[36m[QUERY]\033[0m" << " Client connected: "
                  << addrStr << ":" << ntohs(clientAddr.sin_port) << "\n";

        std::string buffer;
        char recvBuf[4096];

        while (running_.load()) {
            // Set a timeout on recv so we can check running_ periodically
            struct timeval tv;
            tv.tv_sec = 2;
            tv.tv_usec = 0;
            setsockopt(clientSock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

            int bytesRead = recv(clientSock, recvBuf, sizeof(recvBuf) - 1, 0);
            if (bytesRead <= 0) {
                if (bytesRead == 0) break; // Client disconnected
                // Timeout or error - check if still running
                #ifdef _WIN32
                int err = WSAGetLastError();
                if (err == WSAETIMEDOUT) continue;
                #else
                if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
                #endif
                break;
            }

            recvBuf[bytesRead] = '\0';
            buffer += recvBuf;

            // Process complete lines (newline-delimited JSON)
            while (true) {
                size_t nlPos = buffer.find('\n');
                if (nlPos == std::string::npos) break;

                std::string line = buffer.substr(0, nlPos);
                buffer = buffer.substr(nlPos + 1);

                // Trim whitespace
                while (!line.empty() && (line.back() == '\r' || line.back() == ' '))
                    line.pop_back();
                if (line.empty()) continue;

                // Parse request JSON
                std::string type = jsonGetString(line, "type");
                std::string text = jsonGetString(line, "text");
                std::string token = jsonGetString(line, "token");

                // Build response
                std::string response;

                // Auth check
                if (!authToken_.empty() && token != authToken_) {
                    response = "{\"success\":false,\"error\":\"Authentication failed\"}\n";
                    send(clientSock, response.c_str(), response.size(), 0);
                    continue;
                }

                if (type == "query" && !text.empty()) {
                    std::string answer;
                    float confidence = 0.0f;

                    // Route through brain with mutex protection
                    if (brainMutex_ && queryHandler_) {
                        std::lock_guard<std::mutex> lock(*brainMutex_);
                        answer = queryHandler_(text);
                    } else if (queryHandler_) {
                        answer = queryHandler_(text);
                    } else {
                        answer = "Query server has no handler configured.";
                    }

                    // Escape JSON special characters in the answer
                    std::string escaped;
                    escaped.reserve(answer.size() + 32);
                    for (char c : answer) {
                        switch (c) {
                            case '"': escaped += "\\\""; break;
                            case '\\': escaped += "\\\\"; break;
                            case '\n': escaped += "\\n"; break;
                            case '\r': escaped += "\\r"; break;
                            case '\t': escaped += "\\t"; break;
                            default: escaped += c; break;
                        }
                    }

                    response = "{\"success\":true,\"response\":\"" + escaped + "\"}\n";

                } else if (type == "ping") {
                    response = "{\"success\":true,\"response\":\"pong\"}\n";

                } else if (type == "status") {
                    response = "{\"success\":true,\"response\":\"MK Query Server active\"}\n";

                } else {
                    response = "{\"success\":false,\"error\":\"Unknown request type or empty text\"}\n";
                }

                send(clientSock, response.c_str(), response.size(), 0);
            }
        }

        closesocket(clientSock);
        std::cout << "  " << "\033[36m[QUERY]\033[0m" << " Client disconnected: "
                  << addrStr << "\n";
    }

    void listenLoop() {
        while (running_.load()) {
            struct sockaddr_in clientAddr;
            socklen_t addrLen = sizeof(clientAddr);

            // Use select with timeout so we can check running_ flag
            fd_set readSet;
            FD_ZERO(&readSet);
            FD_SET(serverSocket_, &readSet);

            struct timeval tv;
            tv.tv_sec = 1;
            tv.tv_usec = 0;

            int sel = select(serverSocket_ + 1, &readSet, nullptr, nullptr, &tv);
            if (sel <= 0) continue;

            SOCKET clientSock = accept(serverSocket_,
                (struct sockaddr*)&clientAddr, &addrLen);
            if (clientSock == INVALID_SOCKET) continue;

            // Spawn client handler thread
            std::lock_guard<std::mutex> lock(clientsMutex_);
            clientThreads_.emplace_back(&MKQueryServer::handleClient,
                this, clientSock, clientAddr);
        }
    }

public:
    MKQueryServer() : port_(9878), brainMutex_(nullptr) {}

    // Set the port to listen on
    void setPort(int port) { port_ = port; }

    // Set the auth token for client verification
    void setAuthToken(const std::string& token) { authToken_ = token; }

    // Set the external brain mutex for thread-safe query routing
    void setBrainMutex(std::mutex* mtx) { brainMutex_ = mtx; }

    // Set the query handler function (routes through MK's brain)
    void setQueryHandler(QueryHandler handler) { queryHandler_ = std::move(handler); }

    // Start the query server in a background thread
    bool start() {
        #ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "  \033[31m[QUERY]\033[0m WSAStartup failed\n";
            return false;
        }
        #endif

        serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket_ == INVALID_SOCKET) {
            std::cerr << "  \033[31m[QUERY]\033[0m Failed to create socket\n";
            return false;
        }

        // Allow port reuse
        int opt = 1;
        setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR,
            (const char*)&opt, sizeof(opt));

        struct sockaddr_in serverAddr;
        std::memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(port_);

        if (bind(serverSocket_, (struct sockaddr*)&serverAddr,
                 sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << "  \033[31m[QUERY]\033[0m Failed to bind port " << port_ << "\n";
            closesocket(serverSocket_);
            return false;
        }

        if (listen(serverSocket_, 10) == SOCKET_ERROR) {
            std::cerr << "  \033[31m[QUERY]\033[0m Failed to listen\n";
            closesocket(serverSocket_);
            return false;
        }

        running_ = true;
        listenThread_ = std::thread(&MKQueryServer::listenLoop, this);

        std::cout << "  \033[92m✓\033[0m Query server listening on port "
                  << port_ << " (for remote clients)\n";
        return true;
    }

    // Stop the query server
    void stop() {
        if (!running_.load()) return;
        running_ = false;

        if (serverSocket_ != INVALID_SOCKET) {
            closesocket(serverSocket_);
            serverSocket_ = INVALID_SOCKET;
        }

        if (listenThread_.joinable()) {
            listenThread_.join();
        }

        // Join client threads
        std::lock_guard<std::mutex> lock(clientsMutex_);
        for (auto& t : clientThreads_) {
            if (t.joinable()) t.join();
        }
        clientThreads_.clear();

        #ifdef _WIN32
        WSACleanup();
        #endif
    }

    bool isRunning() const { return running_.load(); }
    int getPort() const { return port_; }

    ~MKQueryServer() { stop(); }
};

#endif // MK_QUERY_SERVER_CPP
