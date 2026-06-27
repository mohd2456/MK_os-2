#ifndef MK_NETWORK_DRIVER_CPP
#define MK_NETWORK_DRIVER_CPP

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <array>
#include <cstdio>
#include <map>
#include <cstring>

#ifdef __APPLE__
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if_dl.h>
#endif

#ifdef __linux__
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#endif

struct MKNetInterface {
    std::string name;
    std::string ipAddress;
    std::string macAddress;
    std::string netmask;
    long bytesSent = 0;
    long bytesReceived = 0;
    bool isUp = false;
};

class MKNetworkDriver {
private:
    std::vector<MKNetInterface> interfaces;
    bool connected = false;

    std::string execCommand(const std::string& cmd) const {
        std::array<char, 512> buffer;
        std::string result;
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) return "";
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            result += buffer.data();
        }
        pclose(pipe);
        while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
            result.pop_back();
        }
        return result;
    }

    std::string readFileContent(const std::string& path) const {
        std::ifstream file(path);
        if (!file.is_open()) return "";
        std::string content((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
        while (!content.empty() && (content.back() == '\n' || content.back() == '\r')) {
            content.pop_back();
        }
        return content;
    }

    std::string formatMAC(unsigned char* addr) const {
        char buf[18];
        snprintf(buf, sizeof(buf), "%02x:%02x:%02x:%02x:%02x:%02x",
                 addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
        return std::string(buf);
    }

    void scanInterfaces() {
        interfaces.clear();
        std::map<std::string, MKNetInterface> ifMap;

        struct ifaddrs* ifaddr = nullptr;
        if (getifaddrs(&ifaddr) == -1) return;

        for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr == nullptr) continue;
            std::string name = ifa->ifa_name;

            if (ifMap.find(name) == ifMap.end()) {
                MKNetInterface iface;
                iface.name = name;
                iface.isUp = (ifa->ifa_flags & IFF_UP) != 0;
                ifMap[name] = iface;
            }

            int family = ifa->ifa_addr->sa_family;

            // IPv4 address
            if (family == AF_INET) {
                char host[INET_ADDRSTRLEN];
                struct sockaddr_in* sa = (struct sockaddr_in*)ifa->ifa_addr;
                inet_ntop(AF_INET, &sa->sin_addr, host, sizeof(host));
                ifMap[name].ipAddress = host;

                if (ifa->ifa_netmask) {
                    struct sockaddr_in* mask = (struct sockaddr_in*)ifa->ifa_netmask;
                    char maskBuf[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &mask->sin_addr, maskBuf, sizeof(maskBuf));
                    ifMap[name].netmask = maskBuf;
                }
            }

#ifdef __APPLE__
            // MAC address on macOS
            if (family == AF_LINK) {
                struct sockaddr_dl* sdl = (struct sockaddr_dl*)ifa->ifa_addr;
                if (sdl->sdl_alen == 6) {
                    unsigned char* mac = (unsigned char*)LLADDR(sdl);
                    ifMap[name].macAddress = formatMAC(mac);
                }
            }
#endif

#ifdef __linux__
            // MAC address on Linux
            if (family == AF_PACKET) {
                struct sockaddr_ll* sll = (struct sockaddr_ll*)ifa->ifa_addr;
                if (sll->sll_halen == 6) {
                    ifMap[name].macAddress = formatMAC(sll->sll_addr);
                }
            }
#endif
        }

        freeifaddrs(ifaddr);

        // Get bytes transferred
        for (auto& kv : ifMap) {
            getTrafficStats(kv.second);
            interfaces.push_back(kv.second);
        }
    }

    void getTrafficStats(MKNetInterface& iface) {
#ifdef __APPLE__
        std::string output = execCommand("netstat -bI " + iface.name + " 2>/dev/null | tail -1");
        if (!output.empty()) {
            std::istringstream iss(output);
            std::string name, mtu, net, addr;
            long ipkts, ierrs, ibytes, opkts, oerrs, obytes;
            // Format varies, try to parse bytes columns
            iss >> name;
            // Skip remaining fields and try to extract bytes
            std::string rest;
            std::getline(iss, rest);
            // Simplified: just parse numeric values
            std::istringstream numStream(rest);
            std::vector<long> nums;
            std::string token;
            while (numStream >> token) {
                try {
                    long val = std::stol(token);
                    nums.push_back(val);
                } catch (...) {}
            }
            if (nums.size() >= 6) {
                iface.bytesReceived = nums[2];  // Approximate position
                iface.bytesSent = nums[5];
            }
        }
#elif defined(__linux__)
        std::string rxPath = "/sys/class/net/" + iface.name + "/statistics/rx_bytes";
        std::string txPath = "/sys/class/net/" + iface.name + "/statistics/tx_bytes";
        std::string rx = readFileContent(rxPath);
        std::string tx = readFileContent(txPath);
        if (!rx.empty()) { try { iface.bytesReceived = std::stol(rx); } catch (...) {} }
        if (!tx.empty()) { try { iface.bytesSent = std::stol(tx); } catch (...) {} }
#endif
    }

public:
    MKNetworkDriver() {
        std::cout << "[DRV:NET] Initializing network driver...\n";
        scanInterfaces();
        std::cout << "[DRV:NET] Network driver ready. Found "
                  << interfaces.size() << " interface(s).\n";
    }

    void init() {
        std::cout << "[DRV:NET] Network subsystem initialized.\n";
        stats();
    }

    std::vector<MKNetInterface> listInterfaces() {
        scanInterfaces();
        return interfaces;
    }

    std::string getIPAddress(const std::string& ifName = "") const {
        for (const auto& iface : interfaces) {
            if (ifName.empty()) {
                if (!iface.ipAddress.empty() && iface.ipAddress != "127.0.0.1") {
                    return iface.ipAddress;
                }
            } else if (iface.name == ifName) {
                return iface.ipAddress;
            }
        }
        return "";
    }

    std::string getMACAddress(const std::string& ifName = "") const {
        for (const auto& iface : interfaces) {
            if (ifName.empty()) {
                if (!iface.macAddress.empty() && iface.macAddress != "00:00:00:00:00:00") {
                    return iface.macAddress;
                }
            } else if (iface.name == ifName) {
                return iface.macAddress;
            }
        }
        return "";
    }

    std::pair<long, long> getBytesTransferred(const std::string& ifName) {
        scanInterfaces();
        for (const auto& iface : interfaces) {
            if (iface.name == ifName) {
                return {iface.bytesReceived, iface.bytesSent};
            }
        }
        return {0, 0};
    }

    int getSignalStrength() const {
#ifdef __APPLE__
        // Airport utility RSSI
        std::string output = execCommand(
            "/System/Library/PrivateFrameworks/Apple80211.framework/"
            "Versions/Current/Resources/airport -I 2>/dev/null | grep agrCtlRSSI");
        if (!output.empty()) {
            auto pos = output.find(':');
            if (pos != std::string::npos) {
                try { return std::stoi(output.substr(pos + 1)); } catch (...) {}
            }
        }
        return 0;
#elif defined(__linux__)
        std::string content = readFileContent("/proc/net/wireless");
        if (!content.empty()) {
            std::istringstream stream(content);
            std::string line;
            // Skip header lines
            std::getline(stream, line);
            std::getline(stream, line);
            if (std::getline(stream, line)) {
                std::istringstream lineStream(line);
                std::string iface, status;
                double link, level;
                lineStream >> iface >> status >> link >> level;
                return static_cast<int>(level);
            }
        }
        return 0;
#else
        return 0;
#endif
    }

    bool isConnected() const {
        // Check if we can reach the default gateway
#ifdef __APPLE__
        std::string gw = execCommand("route -n get default 2>/dev/null | grep gateway");
        return !gw.empty();
#elif defined(__linux__)
        std::string gw = execCommand("ip route show default 2>/dev/null | head -1");
        return !gw.empty();
#else
        return false;
#endif
    }

    std::string getConnectionSpeed(const std::string& ifName = "") const {
#ifdef __APPLE__
        if (!ifName.empty()) {
            std::string output = execCommand("ifconfig " + ifName + " 2>/dev/null | grep media");
            if (!output.empty()) return output;
        }
        return "unknown";
#elif defined(__linux__)
        std::string name = ifName.empty() ? "eth0" : ifName;
        std::string speed = readFileContent("/sys/class/net/" + name + "/speed");
        if (!speed.empty()) return speed + " Mbps";
        return "unknown";
#else
        return "unknown";
#endif
    }

    void stats() const {
        std::cout << "[DRV:NET] === Network Statistics ===\n";
        std::cout << "[DRV:NET] Interfaces: " << interfaces.size() << "\n";
        for (const auto& iface : interfaces) {
            std::cout << "[DRV:NET] " << iface.name
                      << " | IP=" << (iface.ipAddress.empty() ? "none" : iface.ipAddress)
                      << " | MAC=" << (iface.macAddress.empty() ? "none" : iface.macAddress)
                      << " | " << (iface.isUp ? "UP" : "DOWN")
                      << " | RX=" << (iface.bytesReceived / 1024) << "KB"
                      << " TX=" << (iface.bytesSent / 1024) << "KB\n";
        }
        int rssi = getSignalStrength();
        if (rssi != 0) {
            std::cout << "[DRV:NET] WiFi Signal: " << rssi << " dBm\n";
        }
        std::cout << "[DRV:NET] Connected: " << (isConnected() ? "Yes" : "No") << "\n";
    }
};

#endif // MK_NETWORK_DRIVER_CPP
