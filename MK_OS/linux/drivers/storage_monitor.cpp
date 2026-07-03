#ifndef MK_LINUX_STORAGE_MONITOR_CPP
#define MK_LINUX_STORAGE_MONITOR_CPP

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <sys/statvfs.h>
#include <cstring>

struct MKDiskStats {
    std::string device;
    std::string mountPoint;
    long totalGB = 0;
    long usedGB = 0;
    long freeGB = 0;
    float usedPercent = 0.0f;
    long readsCompleted = 0;
    long writesCompleted = 0;
    long readSectors = 0;
    long writeSectors = 0;
    long ioTimeMs = 0;
    bool lowSpaceWarning = false;
};

struct MKStorageHealth {
    bool healthy = true;
    bool lowSpace = false;
    float lowestFreePercent = 100.0f;
    std::string warningDevice;
    std::string warningMessage;
};

class MKStorageMonitor {
private:
    std::vector<MKDiskStats> disks;
    static constexpr float LOW_SPACE_THRESHOLD = 10.0f;  // Warn < 10% free

    void readMountPoints() {
        disks.clear();
        std::ifstream mounts("/proc/mounts");
        if (!mounts.is_open()) {
            // Fallback: just check root
            MKDiskStats root = statPath("/", "/dev/root");
            disks.push_back(root);
            return;
        }

        std::string line;
        while (std::getline(mounts, line)) {
            std::istringstream iss(line);
            std::string device, mount, fstype;
            iss >> device >> mount >> fstype;

            // Only track real filesystems
            if (device.find("/dev/") != 0) continue;
            if (fstype == "tmpfs" || fstype == "devtmpfs") continue;
            if (fstype == "squashfs") continue;

            MKDiskStats stats = statPath(mount, device);
            if (stats.totalGB > 0) {
                disks.push_back(stats);
            }
        }
    }

    MKDiskStats statPath(const std::string& path, const std::string& device) const {
        MKDiskStats stats;
        stats.device = device;
        stats.mountPoint = path;

        struct statvfs vfs;
        if (statvfs(path.c_str(), &vfs) == 0) {
            unsigned long blockSize = vfs.f_frsize;
            stats.totalGB = static_cast<long>((vfs.f_blocks * blockSize) / (1024UL * 1024 * 1024));
            stats.freeGB = static_cast<long>((vfs.f_bavail * blockSize) / (1024UL * 1024 * 1024));
            stats.usedGB = stats.totalGB - stats.freeGB;
            if (stats.totalGB > 0) {
                stats.usedPercent = static_cast<float>(stats.usedGB) / static_cast<float>(stats.totalGB) * 100.0f;
            }
            stats.lowSpaceWarning = (100.0f - stats.usedPercent) < LOW_SPACE_THRESHOLD;
        }

        return stats;
    }

    void readIOStats() {
        std::ifstream diskstats("/proc/diskstats");
        if (!diskstats.is_open()) return;

        std::string line;
        while (std::getline(diskstats, line)) {
            std::istringstream iss(line);
            int major, minor;
            std::string devName;
            iss >> major >> minor >> devName;
            (void)major;
            (void)minor;

            // Match to our tracked disks
            for (auto& disk : disks) {
                // Compare device name (e.g., sda1 from /dev/sda1)
                std::string shortDev = disk.device;
                size_t lastSlash = shortDev.rfind('/');
                if (lastSlash != std::string::npos) {
                    shortDev = shortDev.substr(lastSlash + 1);
                }

                if (devName == shortDev) {
                    long val;
                    iss >> disk.readsCompleted;  // field 1
                    iss >> val;                   // field 2: reads merged
                    iss >> disk.readSectors;     // field 3
                    iss >> val;                   // field 4: read time ms
                    iss >> disk.writesCompleted; // field 5
                    iss >> val;                   // field 6: writes merged
                    iss >> disk.writeSectors;    // field 7
                    iss >> val;                   // field 8: write time ms
                    iss >> val;                   // field 9: ios in progress
                    iss >> disk.ioTimeMs;        // field 10
                    break;
                }
            }
        }
    }

public:
    MKStorageMonitor() {
        std::cout << "[LINUX-DRIVER] Storage Monitor initialized\n";
    }

    void scan() {
        readMountPoints();
        readIOStats();
    }

    std::vector<MKDiskStats> getAll() {
        scan();
        return disks;
    }

    MKStorageHealth checkHealth() {
        scan();
        MKStorageHealth health;

        for (const auto& disk : disks) {
            float freePercent = 100.0f - disk.usedPercent;
            if (freePercent < health.lowestFreePercent) {
                health.lowestFreePercent = freePercent;
            }
            if (disk.lowSpaceWarning) {
                health.lowSpace = true;
                health.healthy = false;
                health.warningDevice = disk.device;
                health.warningMessage = "Storage low on " + disk.device +
                    " (" + disk.mountPoint + "): " +
                    std::to_string(static_cast<int>(freePercent)) + "% free";
            }
        }

        return health;
    }

    std::string getStatusJSON() {
        scan();
        std::stringstream ss;
        ss << "{\"disks\":[";
        for (size_t i = 0; i < disks.size(); i++) {
            const auto& d = disks[i];
            ss << "{\"device\":\"" << d.device << "\""
               << ",\"mount\":\"" << d.mountPoint << "\""
               << ",\"total_gb\":" << d.totalGB
               << ",\"used_gb\":" << d.usedGB
               << ",\"free_gb\":" << d.freeGB
               << ",\"used_percent\":" << d.usedPercent
               << ",\"reads\":" << d.readsCompleted
               << ",\"writes\":" << d.writesCompleted
               << ",\"io_time_ms\":" << d.ioTimeMs
               << ",\"low_space\":" << (d.lowSpaceWarning ? "true" : "false")
               << "}";
            if (i < disks.size() - 1) ss << ",";
        }

        auto health = checkHealth();
        ss << "],\"health\":{\"healthy\":" << (health.healthy ? "true" : "false")
           << ",\"low_space\":" << (health.lowSpace ? "true" : "false")
           << ",\"lowest_free_percent\":" << health.lowestFreePercent;
        if (!health.warningMessage.empty()) {
            ss << ",\"warning\":\"" << health.warningMessage << "\"";
        }
        ss << "}}";
        return ss.str();
    }

    bool hasLowSpace() {
        auto health = checkHealth();
        return health.lowSpace;
    }
};

#endif // MK_LINUX_STORAGE_MONITOR_CPP
