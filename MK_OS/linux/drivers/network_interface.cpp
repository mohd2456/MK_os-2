#ifndef MK_LINUX_NETWORK_INTERFACE_CPP
#define MK_LINUX_NETWORK_INTERFACE_CPP

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <dirent.h>

struct MKNetStats {
    std::string name;
    bool isUp = false;
    long rxBytes = 0;
    long txBytes = 0;
    long rxPackets = 0;
    long txPackets = 0;
    long rxErrors = 0;
    long txErrors = 0;
    int mtu = 0;
    std::string ipAddress;
    std::string macAddress;
    std::string operState;
    float bandwidthMbps = 0.0f;
};

class MKNetworkInterface {
private:
    std::vector<MKNetStats> interfaces;
    std::map<std::string, long> prevRxBytes;
    std::map<std::string, long> prevTxBytes;

    std::string readSysFile(const std::string& path) const {
        std::ifstream file(path);
        if (!file.is_open()) return "";
        std::string content;
        std::getline(file, content);
        while (!content.empty() && (content.back() == '\n' || content.back() == '\r')) {
            content.pop_back();
        }
        return content;
    }

    long readSysFileLong(const std::string& path) const {
        std::string val = readSysFile(path);
        if (val.empty()) return 0;
        try {
            return std::stol(val);
        } catch (...) {
            return 0;
        }
    }

    MKNetStats readInterface(const std::string& ifName) const {
        MKNetStats stats;
        stats.name = ifName;
        std::string base = "/sys/class/net/" + ifName + "/";

        // Operational state
        stats.operState = readSysFile(base + "operstate");
        stats.isUp = (stats.operState == "up");

        // MTU
        stats.mtu = static_cast<int>(readSysFileLong(base + "mtu"));

        // MAC address
        stats.macAddress = readSysFile(base + "address");

        // Traffic statistics
        std::string statsBase = base + "statistics/";
        stats.rxBytes = readSysFileLong(statsBase + "rx_bytes");
        stats.txBytes = readSysFileLong(statsBase + "tx_bytes");
        stats.rxPackets = readSysFileLong(statsBase + "rx_packets");
        stats.txPackets = readSysFileLong(statsBase + "tx_packets");
        stats.rxErrors = readSysFileLong(statsBase + "rx_errors");
        stats.txErrors = readSysFileLong(statsBase + "tx_errors");

        // Speed (in Mbps, from /sys/class/net/<iface>/speed)
        long speed = readSysFileLong(base + "speed");
        if (speed > 0) {
            stats.bandwidthMbps = static_cast<float>(speed);
        }

        return stats;
    }

public:
    MKNetworkInterface() {
        std::cout << "[LINUX-DRIVER] Network Interface monitor initialized\n";
    }

    void scan() {
        interfaces.clear();
        std::string netPath = "/sys/class/net/";
        DIR* dir = opendir(netPath.c_str());
        if (!dir) {
            // Fallback: try to read /proc/net/dev
            scanFromProc();
            return;
        }

        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string name = entry->d_name;
            if (name == "." || name == "..") continue;
            if (name == "lo") continue;  // Skip loopback
            interfaces.push_back(readInterface(name));
        }
        closedir(dir);
    }

    void scanFromProc() {
        std::ifstream proc("/proc/net/dev");
        if (!proc.is_open()) return;

        std::string line;
        // Skip first two header lines
        std::getline(proc, line);
        std::getline(proc, line);

        while (std::getline(proc, line)) {
            std::istringstream iss(line);
            std::string ifName;
            iss >> ifName;
            if (ifName.empty()) continue;
            // Remove trailing colon
            if (ifName.back() == ':') ifName.pop_back();
            if (ifName == "lo") continue;

            MKNetStats stats;
            stats.name = ifName;
            iss >> stats.rxBytes >> stats.rxPackets >> stats.rxErrors;
            // Skip remaining rx fields
            long dummy;
            iss >> dummy >> dummy >> dummy >> dummy >> dummy;
            iss >> stats.txBytes >> stats.txPackets >> stats.txErrors;

            stats.isUp = (stats.rxBytes > 0 || stats.txBytes > 0);
            interfaces.push_back(stats);
        }
    }

    std::vector<MKNetStats> getAll() {
        scan();
        return interfaces;
    }

    MKNetStats getInterface(const std::string& name) {
        scan();
        for (const auto& iface : interfaces) {
            if (iface.name == name) return iface;
        }
        MKNetStats empty;
        empty.name = name;
        return empty;
    }

    std::string getStatusJSON() {
        scan();
        std::stringstream ss;
        ss << "[";
        for (size_t i = 0; i < interfaces.size(); i++) {
            const auto& iface = interfaces[i];
            ss << "{\"name\":\"" << iface.name << "\""
               << ",\"up\":" << (iface.isUp ? "true" : "false")
               << ",\"mac\":\"" << iface.macAddress << "\""
               << ",\"mtu\":" << iface.mtu
               << ",\"rx_bytes\":" << iface.rxBytes
               << ",\"tx_bytes\":" << iface.txBytes
               << ",\"rx_packets\":" << iface.rxPackets
               << ",\"tx_packets\":" << iface.txPackets
               << ",\"rx_errors\":" << iface.rxErrors
               << ",\"tx_errors\":" << iface.txErrors
               << ",\"bandwidth_mbps\":" << iface.bandwidthMbps
               << ",\"oper_state\":\"" << iface.operState << "\""
               << "}";
            if (i < interfaces.size() - 1) ss << ",";
        }
        ss << "]";
        return ss.str();
    }

    bool hasActiveInterface() {
        scan();
        for (const auto& iface : interfaces) {
            if (iface.isUp) return true;
        }
        return false;
    }

    // Get total bandwidth usage across all interfaces (bytes/sec estimation)
    long getTotalRxRate() {
        scan();
        long totalDiff = 0;
        for (const auto& iface : interfaces) {
            auto it = prevRxBytes.find(iface.name);
            if (it != prevRxBytes.end()) {
                long diff = iface.rxBytes - it->second;
                if (diff > 0) totalDiff += diff;
            }
            prevRxBytes[iface.name] = iface.rxBytes;
        }
        return totalDiff;
    }

    long getTotalTxRate() {
        long totalDiff = 0;
        for (const auto& iface : interfaces) {
            auto it = prevTxBytes.find(iface.name);
            if (it != prevTxBytes.end()) {
                long diff = iface.txBytes - it->second;
                if (diff > 0) totalDiff += diff;
            }
            prevTxBytes[iface.name] = iface.txBytes;
        }
        return totalDiff;
    }
};

#endif // MK_LINUX_NETWORK_INTERFACE_CPP
