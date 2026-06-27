#ifndef MK_CPU_DRIVER_CPP
#define MK_CPU_DRIVER_CPP

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <array>
#include <cstdio>
#include <map>
#include <algorithm>
#include <cstring>

#ifdef __APPLE__
#include <sys/sysctl.h>
#include <mach/mach.h>
#include <mach/processor_info.h>
#include <mach/mach_host.h>
#endif

struct MKCPUCacheInfo {
    long l1d = 0;  // L1 data cache (bytes)
    long l1i = 0;  // L1 instruction cache (bytes)
    long l2 = 0;   // L2 cache (bytes)
    long l3 = 0;   // L3 cache (bytes)
};

struct MKCPUFrequency {
    long current = 0;  // MHz
    long max = 0;      // MHz
    long min = 0;      // MHz
};

struct MKCoreUsage {
    int coreId = 0;
    double usagePercent = 0.0;
};

class MKCPUDriver {
private:
    std::string modelName;
    int coreCount = 0;
    MKCPUFrequency frequency;
    MKCPUCacheInfo cacheInfo;
    std::vector<std::string> features;
    std::vector<MKCoreUsage> coreUsages;

    // Execute a shell command and return output
    std::string execCommand(const std::string& cmd) const {
        std::array<char, 256> buffer;
        std::string result;
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) return "";
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            result += buffer.data();
        }
        pclose(pipe);
        // Trim trailing newline
        while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
            result.pop_back();
        }
        return result;
    }

    std::string readFile(const std::string& path) const {
        std::ifstream file(path);
        if (!file.is_open()) return "";
        std::string content((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
        while (!content.empty() && (content.back() == '\n' || content.back() == '\r')) {
            content.pop_back();
        }
        return content;
    }

#ifdef __APPLE__
    std::string sysctlString(const std::string& name) const {
        char buf[256];
        size_t len = sizeof(buf);
        if (sysctlbyname(name.c_str(), buf, &len, nullptr, 0) == 0) {
            return std::string(buf, len > 0 ? len - 1 : 0);
        }
        return "";
    }

    long sysctlLong(const std::string& name) const {
        int64_t val = 0;
        size_t len = sizeof(val);
        if (sysctlbyname(name.c_str(), &val, &len, nullptr, 0) == 0) {
            return static_cast<long>(val);
        }
        return 0;
    }
#endif

    void detectModel() {
#ifdef __APPLE__
        modelName = sysctlString("machdep.cpu.brand_string");
#elif defined(__linux__)
        std::ifstream cpuinfo("/proc/cpuinfo");
        std::string line;
        while (std::getline(cpuinfo, line)) {
            if (line.find("model name") != std::string::npos) {
                auto pos = line.find(':');
                if (pos != std::string::npos) {
                    modelName = line.substr(pos + 2);
                }
                break;
            }
        }
#endif
        if (modelName.empty()) modelName = "Unknown CPU";
    }

    void detectCoreCount() {
#ifdef __APPLE__
        coreCount = static_cast<int>(sysctlLong("hw.physicalcpu"));
        if (coreCount == 0) coreCount = static_cast<int>(sysctlLong("hw.ncpu"));
#elif defined(__linux__)
        std::string result = readFile("/proc/cpuinfo");
        int count = 0;
        std::istringstream stream(result);
        std::string line;
        while (std::getline(stream, line)) {
            if (line.find("processor") == 0) count++;
        }
        coreCount = count > 0 ? count : 1;
#endif
    }

    void detectFrequency() {
#ifdef __APPLE__
        long freqHz = sysctlLong("hw.cpufrequency");
        long freqMaxHz = sysctlLong("hw.cpufrequency_max");
        long freqMinHz = sysctlLong("hw.cpufrequency_min");
        frequency.current = freqHz / 1000000;
        frequency.max = freqMaxHz / 1000000;
        frequency.min = freqMinHz / 1000000;
        if (frequency.max == 0) frequency.max = frequency.current;
        if (frequency.min == 0) frequency.min = frequency.current;
#elif defined(__linux__)
        std::string cur = readFile("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq");
        std::string maxf = readFile("/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq");
        std::string minf = readFile("/sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq");
        if (!cur.empty()) frequency.current = std::stol(cur) / 1000;
        if (!maxf.empty()) frequency.max = std::stol(maxf) / 1000;
        if (!minf.empty()) frequency.min = std::stol(minf) / 1000;
        // Fallback from /proc/cpuinfo
        if (frequency.current == 0) {
            std::ifstream cpuinfo("/proc/cpuinfo");
            std::string line;
            while (std::getline(cpuinfo, line)) {
                if (line.find("cpu MHz") != std::string::npos) {
                    auto pos = line.find(':');
                    if (pos != std::string::npos) {
                        frequency.current = static_cast<long>(std::stod(line.substr(pos + 2)));
                    }
                    break;
                }
            }
        }
#endif
    }

    void detectCache() {
#ifdef __APPLE__
        cacheInfo.l1d = sysctlLong("hw.l1dcachesize");
        cacheInfo.l1i = sysctlLong("hw.l1icachesize");
        cacheInfo.l2 = sysctlLong("hw.l2cachesize");
        cacheInfo.l3 = sysctlLong("hw.l3cachesize");
#elif defined(__linux__)
        // Try sysfs cache topology
        for (int i = 0; i < 4; i++) {
            std::string base = "/sys/devices/system/cpu/cpu0/cache/index" + std::to_string(i);
            std::string level = readFile(base + "/level");
            std::string type = readFile(base + "/type");
            std::string sizeStr = readFile(base + "/size");
            if (sizeStr.empty()) continue;
            long size = 0;
            if (sizeStr.back() == 'K') {
                size = std::stol(sizeStr.substr(0, sizeStr.size() - 1)) * 1024;
            } else if (sizeStr.back() == 'M') {
                size = std::stol(sizeStr.substr(0, sizeStr.size() - 1)) * 1024 * 1024;
            } else {
                size = std::stol(sizeStr);
            }
            if (level == "1" && type == "Data") cacheInfo.l1d = size;
            else if (level == "1" && type == "Instruction") cacheInfo.l1i = size;
            else if (level == "2") cacheInfo.l2 = size;
            else if (level == "3") cacheInfo.l3 = size;
        }
#endif
    }

    void detectFeatures() {
#ifdef __APPLE__
        std::string featureStr = sysctlString("machdep.cpu.features");
        std::istringstream stream(featureStr);
        std::string token;
        while (stream >> token) {
            features.push_back(token);
        }
        // Also check extended features
        std::string extFeatures = sysctlString("machdep.cpu.leaf7_features");
        std::istringstream extStream(extFeatures);
        while (extStream >> token) {
            features.push_back(token);
        }
#elif defined(__linux__)
        std::ifstream cpuinfo("/proc/cpuinfo");
        std::string line;
        while (std::getline(cpuinfo, line)) {
            if (line.find("flags") == 0) {
                auto pos = line.find(':');
                if (pos != std::string::npos) {
                    std::istringstream flagStream(line.substr(pos + 2));
                    std::string flag;
                    while (flagStream >> flag) {
                        features.push_back(flag);
                    }
                }
                break;
            }
        }
#endif
    }

public:
    MKCPUDriver() {
        std::cout << "[DRV:CPU] Initializing CPU driver...\n";
        detectModel();
        detectCoreCount();
        detectFrequency();
        detectCache();
        detectFeatures();
        std::cout << "[DRV:CPU] CPU driver ready: " << modelName << "\n";
    }

    void init() {
        std::cout << "[DRV:CPU] CPU subsystem initialized.\n";
        std::cout << "[DRV:CPU] Model: " << modelName << "\n";
        std::cout << "[DRV:CPU] Cores: " << coreCount << "\n";
        std::cout << "[DRV:CPU] Frequency: " << frequency.current << " MHz"
                  << " (min=" << frequency.min << ", max=" << frequency.max << ")\n";
    }

    std::string getModel() const { return modelName; }

    int getCoreCount() const { return coreCount; }

    MKCPUFrequency getFrequency() const { return frequency; }

    double getTemperature() const {
#ifdef __APPLE__
        // macOS: try SMC via sysctl (limited access without root)
        std::string result = execCommand("sysctl -n machdep.xcpm.cpu_thermal_level 2>/dev/null");
        if (!result.empty()) {
            try { return std::stod(result); } catch (...) {}
        }
        return -1.0;  // Not available without IOKit/SMC access
#elif defined(__linux__)
        // Try thermal zone
        for (int i = 0; i < 10; i++) {
            std::string path = "/sys/class/thermal/thermal_zone" + std::to_string(i) + "/temp";
            std::string val = readFile(path);
            if (!val.empty()) {
                try {
                    long temp = std::stol(val);
                    return temp / 1000.0;  // Convert millidegrees to degrees
                } catch (...) {}
            }
        }
        return -1.0;
#else
        return -1.0;
#endif
    }

    std::vector<MKCoreUsage> getUsage() {
        coreUsages.clear();
#ifdef __APPLE__
        processor_info_array_t cpuInfo;
        mach_msg_type_number_t numCpuInfo;
        natural_t numCPUs = 0;
        kern_return_t kr = host_processor_info(mach_host_self(),
                                                PROCESSOR_CPU_LOAD_INFO,
                                                &numCPUs,
                                                &cpuInfo,
                                                &numCpuInfo);
        if (kr == KERN_SUCCESS) {
            for (natural_t i = 0; i < numCPUs; i++) {
                int idx = static_cast<int>(CPU_STATE_MAX * i);
                double user = cpuInfo[idx + CPU_STATE_USER];
                double sys = cpuInfo[idx + CPU_STATE_SYSTEM];
                double idle = cpuInfo[idx + CPU_STATE_IDLE];
                double nice = cpuInfo[idx + CPU_STATE_NICE];
                double total = user + sys + idle + nice;
                double usage = (total > 0) ? ((user + sys + nice) / total) * 100.0 : 0.0;
                coreUsages.push_back({static_cast<int>(i), usage});
            }
            vm_deallocate(mach_task_self(), (vm_address_t)cpuInfo,
                         numCpuInfo * sizeof(integer_t));
        }
#elif defined(__linux__)
        std::ifstream statFile("/proc/stat");
        std::string line;
        while (std::getline(statFile, line)) {
            if (line.substr(0, 3) == "cpu" && line[3] != ' ') {
                std::istringstream iss(line);
                std::string cpuLabel;
                long user, nice, system, idle, iowait, irq, softirq;
                iss >> cpuLabel >> user >> nice >> system >> idle >> iowait >> irq >> softirq;
                long total = user + nice + system + idle + iowait + irq + softirq;
                long active = user + nice + system + irq + softirq;
                double usage = (total > 0) ? (static_cast<double>(active) / total) * 100.0 : 0.0;
                int id = std::stoi(cpuLabel.substr(3));
                coreUsages.push_back({id, usage});
            }
        }
#endif
        return coreUsages;
    }

    MKCPUCacheInfo getCacheInfo() const { return cacheInfo; }

    std::vector<std::string> getFeatures() const { return features; }

    bool hasFeature(const std::string& feat) const {
        std::string upperFeat = feat;
        std::transform(upperFeat.begin(), upperFeat.end(), upperFeat.begin(), ::toupper);
        for (const auto& f : features) {
            std::string upper = f;
            std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
            if (upper == upperFeat) return true;
        }
        return false;
    }

    std::string getFrequencyScaling() const {
#ifdef __APPLE__
        return execCommand("sysctl -n machdep.xcpm.mode 2>/dev/null");
#elif defined(__linux__)
        return readFile("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
#else
        return "unknown";
#endif
    }

    void stats() const {
        std::cout << "[DRV:CPU] === CPU Statistics ===\n";
        std::cout << "[DRV:CPU] Model: " << modelName << "\n";
        std::cout << "[DRV:CPU] Cores: " << coreCount << "\n";
        std::cout << "[DRV:CPU] Frequency: " << frequency.current << " MHz"
                  << " (range: " << frequency.min << "-" << frequency.max << " MHz)\n";
        std::cout << "[DRV:CPU] Cache L1d=" << (cacheInfo.l1d / 1024) << "KB"
                  << " L1i=" << (cacheInfo.l1i / 1024) << "KB"
                  << " L2=" << (cacheInfo.l2 / 1024) << "KB"
                  << " L3=" << (cacheInfo.l3 / 1024 / 1024) << "MB\n";
        double temp = getTemperature();
        if (temp >= 0) {
            std::cout << "[DRV:CPU] Temperature: " << temp << " C\n";
        }
        std::cout << "[DRV:CPU] Features (" << features.size() << "): ";
        int shown = 0;
        for (const auto& f : features) {
            if (shown++ < 10) std::cout << f << " ";
        }
        if (features.size() > 10) std::cout << "...";
        std::cout << "\n";
    }
};

#endif // MK_CPU_DRIVER_CPP
