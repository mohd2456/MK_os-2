#ifndef MK_MEMORY_DRIVER_CPP
#define MK_MEMORY_DRIVER_CPP

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <array>
#include <cstdio>
#include <map>

#ifdef __APPLE__
#include <sys/sysctl.h>
#include <mach/mach.h>
#include <mach/vm_statistics.h>
#include <mach/mach_host.h>
#endif

struct MKSwapInfo {
    long totalMB = 0;
    long usedMB = 0;
    long freeMB = 0;
};

struct MKMemoryStats {
    long totalMB = 0;
    long usedMB = 0;
    long freeMB = 0;
    long availableMB = 0;
    long buffersMB = 0;
    long cachedMB = 0;
};

class MKMemoryDriver {
private:
    MKMemoryStats memStats;
    MKSwapInfo swapInfo;

    std::string execCommand(const std::string& cmd) const {
        std::array<char, 256> buffer;
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
        return content;
    }

#ifdef __APPLE__
    long sysctlLong(const std::string& name) const {
        int64_t val = 0;
        size_t len = sizeof(val);
        if (sysctlbyname(name.c_str(), &val, &len, nullptr, 0) == 0) {
            return static_cast<long>(val);
        }
        return 0;
    }
#endif

    void refreshMemory() {
#ifdef __APPLE__
        long totalBytes = sysctlLong("hw.memsize");
        memStats.totalMB = totalBytes / (1024 * 1024);

        // Get VM statistics for used/free breakdown
        vm_size_t pageSize;
        mach_port_t hostPort = mach_host_self();
        host_page_size(hostPort, &pageSize);

        vm_statistics64_data_t vmStats;
        mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
        kern_return_t kr = host_statistics64(hostPort, HOST_VM_INFO64,
                                             (host_info64_t)&vmStats, &count);
        if (kr == KERN_SUCCESS) {
            long freePages = vmStats.free_count;
            long activePages = vmStats.active_count;
            long inactivePages = vmStats.inactive_count;
            long wiredPages = vmStats.wire_count;
            long compressedPages = vmStats.compressor_page_count;

            memStats.freeMB = static_cast<long>((freePages * pageSize) / (1024 * 1024));
            memStats.cachedMB = static_cast<long>((inactivePages * pageSize) / (1024 * 1024));
            long usedPages = activePages + wiredPages + compressedPages;
            memStats.usedMB = static_cast<long>((usedPages * pageSize) / (1024 * 1024));
            memStats.availableMB = memStats.totalMB - memStats.usedMB;
            memStats.buffersMB = static_cast<long>((compressedPages * pageSize) / (1024 * 1024));
        }

        // Swap info from sysctl
        struct xsw_usage swapUsage;
        size_t swapLen = sizeof(swapUsage);
        if (sysctlbyname("vm.swapusage", &swapUsage, &swapLen, nullptr, 0) == 0) {
            swapInfo.totalMB = static_cast<long>(swapUsage.xsu_total / (1024 * 1024));
            swapInfo.usedMB = static_cast<long>(swapUsage.xsu_used / (1024 * 1024));
            swapInfo.freeMB = static_cast<long>(swapUsage.xsu_avail / (1024 * 1024));
        }
#elif defined(__linux__)
        std::string content = readFileContent("/proc/meminfo");
        std::map<std::string, long> memMap;
        std::istringstream stream(content);
        std::string line;
        while (std::getline(stream, line)) {
            std::istringstream lineStream(line);
            std::string key;
            long value;
            lineStream >> key >> value;
            // Remove trailing colon from key
            if (!key.empty() && key.back() == ':') key.pop_back();
            memMap[key] = value;  // Values are in kB
        }

        memStats.totalMB = memMap["MemTotal"] / 1024;
        memStats.freeMB = memMap["MemFree"] / 1024;
        memStats.availableMB = memMap["MemAvailable"] / 1024;
        memStats.buffersMB = memMap["Buffers"] / 1024;
        memStats.cachedMB = memMap["Cached"] / 1024;
        memStats.usedMB = memStats.totalMB - memStats.freeMB - memStats.buffersMB - memStats.cachedMB;

        swapInfo.totalMB = memMap["SwapTotal"] / 1024;
        swapInfo.freeMB = memMap["SwapFree"] / 1024;
        swapInfo.usedMB = swapInfo.totalMB - swapInfo.freeMB;
#endif
    }

public:
    MKMemoryDriver() {
        std::cout << "[DRV:MEM] Initializing memory driver...\n";
        refreshMemory();
        std::cout << "[DRV:MEM] Memory driver ready. Total RAM: "
                  << memStats.totalMB << " MB\n";
    }

    void init() {
        std::cout << "[DRV:MEM] Memory subsystem initialized.\n";
        stats();
    }

    long getTotalRAM() {
        refreshMemory();
        return memStats.totalMB;
    }

    long getUsedRAM() {
        refreshMemory();
        return memStats.usedMB;
    }

    long getFreeRAM() {
        refreshMemory();
        return memStats.freeMB;
    }

    long getAvailableRAM() {
        refreshMemory();
        return memStats.availableMB;
    }

    MKSwapInfo getSwapUsage() {
        refreshMemory();
        return swapInfo;
    }

    std::string getMemoryPressure() const {
#ifdef __APPLE__
        // macOS memory pressure level
        std::string result = execCommand("sysctl -n kern.memorystatus_vm_pressure_level 2>/dev/null");
        if (!result.empty()) {
            int level = 0;
            try { level = std::stoi(result); } catch (...) {}
            switch (level) {
                case 1: return "normal";
                case 2: return "warning";
                case 4: return "critical";
                default: return "normal";
            }
        }
        return "unknown";
#elif defined(__linux__)
        std::string content = readFileContent("/proc/pressure/memory");
        if (!content.empty()) {
            // Parse PSI (Pressure Stall Information)
            // Format: some avg10=X.XX avg60=X.XX avg300=X.XX total=N
            std::istringstream stream(content);
            std::string line;
            if (std::getline(stream, line)) {
                auto pos = line.find("avg10=");
                if (pos != std::string::npos) {
                    std::string val = line.substr(pos + 6);
                    auto spacePos = val.find(' ');
                    if (spacePos != std::string::npos) val = val.substr(0, spacePos);
                    try {
                        double pressure = std::stod(val);
                        if (pressure < 10.0) return "normal";
                        if (pressure < 40.0) return "warning";
                        return "critical";
                    } catch (...) {}
                }
            }
        }
        return "unknown";
#else
        return "unknown";
#endif
    }

    long getPageFaults() const {
#ifdef __APPLE__
        vm_statistics64_data_t vmStats;
        mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
        kern_return_t kr = host_statistics64(mach_host_self(), HOST_VM_INFO64,
                                             (host_info64_t)&vmStats, &count);
        if (kr == KERN_SUCCESS) {
            return static_cast<long>(vmStats.faults);
        }
        return 0;
#elif defined(__linux__)
        std::string content = readFileContent("/proc/vmstat");
        std::istringstream stream(content);
        std::string key;
        long value;
        while (stream >> key >> value) {
            if (key == "pgfault") return value;
        }
        return 0;
#else
        return 0;
#endif
    }

    MKMemoryStats getFullStats() {
        refreshMemory();
        return memStats;
    }

    void stats() const {
        std::cout << "[DRV:MEM] === Memory Statistics ===\n";
        std::cout << "[DRV:MEM] Total: " << memStats.totalMB << " MB\n";
        std::cout << "[DRV:MEM] Used: " << memStats.usedMB << " MB\n";
        std::cout << "[DRV:MEM] Free: " << memStats.freeMB << " MB\n";
        std::cout << "[DRV:MEM] Available: " << memStats.availableMB << " MB\n";
        std::cout << "[DRV:MEM] Buffers: " << memStats.buffersMB << " MB\n";
        std::cout << "[DRV:MEM] Cached: " << memStats.cachedMB << " MB\n";
        std::cout << "[DRV:MEM] Swap: " << swapInfo.usedMB << "/"
                  << swapInfo.totalMB << " MB used\n";
        std::cout << "[DRV:MEM] Pressure: " << getMemoryPressure() << "\n";
        std::cout << "[DRV:MEM] Page Faults: " << getPageFaults() << "\n";
    }
};

#endif // MK_MEMORY_DRIVER_CPP
