#ifndef MK_LINUX_GPU_MONITOR_CPP
#define MK_LINUX_GPU_MONITOR_CPP

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdio>
#include <array>
#include <dirent.h>

struct MKGPUStats {
    std::string gpuName = "Unknown";
    std::string driver = "unknown";
    float temperatureC = -1.0f;
    float usagePercent = -1.0f;
    long vramUsedMB = -1;
    long vramTotalMB = -1;
    float powerWatts = -1.0f;
    bool available = false;
};

class MKGPUMonitor {
private:
    std::vector<MKGPUStats> gpus;

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

    long readSysFileInt(const std::string& path) const {
        std::string val = readSysFile(path);
        if (val.empty()) return -1;
        try {
            return std::stol(val);
        } catch (...) {
            return -1;
        }
    }

    // Detect AMD GPU via /sys/class/drm/
    MKGPUStats detectAMDGPU() const {
        MKGPUStats stats;
        std::string basePath = "/sys/class/drm/card0/device/";

        // Check if amdgpu driver is loaded
        std::string driverLink = basePath + "driver";
        std::ifstream driverTest(basePath + "vendor");
        if (!driverTest.is_open()) return stats;

        std::string vendor = readSysFile(basePath + "vendor");
        // AMD vendor ID: 0x1002
        if (vendor.find("1002") == std::string::npos) return stats;

        stats.available = true;
        stats.driver = "amdgpu";
        stats.gpuName = "AMD GPU";

        // Temperature: hwmon interface
        std::string hwmonBase = basePath + "hwmon/";
        DIR* dir = opendir(hwmonBase.c_str());
        if (dir) {
            struct dirent* entry;
            while ((entry = readdir(dir)) != nullptr) {
                std::string name = entry->d_name;
                if (name.find("hwmon") != std::string::npos) {
                    std::string tempPath = hwmonBase + name + "/temp1_input";
                    long tempRaw = readSysFileInt(tempPath);
                    if (tempRaw > 0) {
                        stats.temperatureC = static_cast<float>(tempRaw) / 1000.0f;
                    }
                    // Power
                    std::string powerPath = hwmonBase + name + "/power1_average";
                    long powerRaw = readSysFileInt(powerPath);
                    if (powerRaw > 0) {
                        stats.powerWatts = static_cast<float>(powerRaw) / 1000000.0f;
                    }
                    break;
                }
            }
            closedir(dir);
        }

        // GPU usage via gpu_busy_percent
        std::string busyPath = basePath + "gpu_busy_percent";
        long busy = readSysFileInt(busyPath);
        if (busy >= 0) {
            stats.usagePercent = static_cast<float>(busy);
        }

        // VRAM via mem_info_vram_total/used
        long vramTotal = readSysFileInt(basePath + "mem_info_vram_total");
        long vramUsed = readSysFileInt(basePath + "mem_info_vram_used");
        if (vramTotal > 0) {
            stats.vramTotalMB = vramTotal / (1024 * 1024);
        }
        if (vramUsed > 0) {
            stats.vramUsedMB = vramUsed / (1024 * 1024);
        }

        return stats;
    }

    // Detect Intel GPU via /sys/class/drm/
    MKGPUStats detectIntelGPU() const {
        MKGPUStats stats;
        std::string basePath = "/sys/class/drm/card0/device/";

        std::string vendor = readSysFile(basePath + "vendor");
        // Intel vendor ID: 0x8086
        if (vendor.find("8086") == std::string::npos) return stats;

        stats.available = true;
        stats.driver = "i915";
        stats.gpuName = "Intel Integrated Graphics";

        // Temperature from i915 hwmon
        std::string hwmonBase = basePath + "hwmon/";
        DIR* dir = opendir(hwmonBase.c_str());
        if (dir) {
            struct dirent* entry;
            while ((entry = readdir(dir)) != nullptr) {
                std::string name = entry->d_name;
                if (name.find("hwmon") != std::string::npos) {
                    std::string tempPath = hwmonBase + name + "/temp1_input";
                    long tempRaw = readSysFileInt(tempPath);
                    if (tempRaw > 0) {
                        stats.temperatureC = static_cast<float>(tempRaw) / 1000.0f;
                    }
                    break;
                }
            }
            closedir(dir);
        }

        return stats;
    }

public:
    MKGPUMonitor() {
        std::cout << "[LINUX-DRIVER] GPU Monitor initialized\n";
    }

    void scan() {
        gpus.clear();

        // Try AMD first
        MKGPUStats amd = detectAMDGPU();
        if (amd.available) {
            gpus.push_back(amd);
        }

        // Try Intel
        MKGPUStats intel = detectIntelGPU();
        if (intel.available) {
            gpus.push_back(intel);
        }

        // If nothing detected, create a placeholder
        if (gpus.empty()) {
            MKGPUStats placeholder;
            placeholder.gpuName = "No GPU detected (or sysfs not available)";
            placeholder.available = false;
            gpus.push_back(placeholder);
        }
    }

    std::vector<MKGPUStats> getStats() {
        scan();
        return gpus;
    }

    std::string getStatusJSON() {
        scan();
        std::stringstream ss;
        ss << "[";
        for (size_t i = 0; i < gpus.size(); i++) {
            const auto& g = gpus[i];
            ss << "{\"name\":\"" << g.gpuName << "\""
               << ",\"driver\":\"" << g.driver << "\""
               << ",\"temperature_c\":" << g.temperatureC
               << ",\"usage_percent\":" << g.usagePercent
               << ",\"vram_used_mb\":" << g.vramUsedMB
               << ",\"vram_total_mb\":" << g.vramTotalMB
               << ",\"power_watts\":" << g.powerWatts
               << ",\"available\":" << (g.available ? "true" : "false")
               << "}";
            if (i < gpus.size() - 1) ss << ",";
        }
        ss << "]";
        return ss.str();
    }

    bool isGPUAvailable() const {
        for (const auto& g : gpus) {
            if (g.available) return true;
        }
        return false;
    }

    bool isSafeForGPUTasks() {
        scan();
        for (const auto& g : gpus) {
            if (!g.available) continue;
            // Unsafe if temp > 85C or usage > 90%
            if (g.temperatureC > 85.0f) return false;
            if (g.usagePercent > 90.0f) return false;
        }
        return true;
    }
};

#endif // MK_LINUX_GPU_MONITOR_CPP
