#ifndef MK_RESOURCE_MONITOR_CPP
#define MK_RESOURCE_MONITOR_CPP

#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <cstdio>
#include <array>

// ===================================================================================
// MK RESOURCE MONITOR
// Checks device resources BEFORE running tasks. Prevents crashes by refusing tasks
// that would overload a device. Implements the "don't crash" philosophy.
// Reads /proc/cpuinfo, /proc/meminfo, /sys/class/thermal/ on Linux.
// On macOS (Darwin), uses sysctl/vm_stat equivalents.
// ===================================================================================

struct MKResourceSnapshot {
    float cpu_usage_percent;     // Current CPU usage 0-100
    long total_ram_mb;
    long available_ram_mb;
    long total_disk_mb;
    long available_disk_mb;
    float temperature_celsius;
    int cpu_cores;
    float load_average_1m;
    float load_average_5m;
    float load_average_15m;
    std::time_t timestamp;
    bool valid;
};

struct MKTaskRequirements {
    float cpu_percent_needed;    // How much CPU this task needs (0-100)
    long ram_mb_needed;
    long disk_mb_needed;
    float max_temperature;       // Don't run if device is hotter than this
    std::string description;
};

struct MKResourceThresholds {
    float max_cpu_percent;       // Don't exceed this CPU usage
    float max_ram_percent;       // Don't use more than this % of RAM
    float max_temperature;       // Hard stop temperature
    float warning_temperature;   // Start throttling here
    long min_free_disk_mb;       // Always keep this much disk free
    float safety_margin;         // Extra margin (0.1 = 10% extra buffer)
};

class MKResourceMonitor {
private:
    MKResourceThresholds thresholds;
    std::map<std::string, MKResourceSnapshot> device_snapshots;
    std::vector<std::string> overload_log;

    // Execute a command and capture output (platform independent)
    std::string execCmd(const std::string& cmd) const {
        std::string result;
        std::array<char, 256> buffer;
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) return "";
        while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
            result += buffer.data();
        }
        pclose(pipe);
        return result;
    }

    // Read local system resources (Linux)
    MKResourceSnapshot readLocalLinux() const {
        MKResourceSnapshot snap;
        snap.valid = false;
        snap.timestamp = std::time(nullptr);
        snap.cpu_usage_percent = 0.0f;
        snap.total_ram_mb = 0;
        snap.available_ram_mb = 0;
        snap.total_disk_mb = 0;
        snap.available_disk_mb = 0;
        snap.temperature_celsius = 0.0f;
        snap.cpu_cores = 0;
        snap.load_average_1m = 0.0f;
        snap.load_average_5m = 0.0f;
        snap.load_average_15m = 0.0f;

        // Read /proc/meminfo
        std::ifstream meminfo("/proc/meminfo");
        if (meminfo.is_open()) {
            std::string line;
            while (std::getline(meminfo, line)) {
                if (line.find("MemTotal:") == 0) {
                    std::istringstream iss(line.substr(9));
                    long kb; iss >> kb;
                    snap.total_ram_mb = kb / 1024;
                } else if (line.find("MemAvailable:") == 0) {
                    std::istringstream iss(line.substr(13));
                    long kb; iss >> kb;
                    snap.available_ram_mb = kb / 1024;
                }
            }
            meminfo.close();
        }

        // Read /proc/cpuinfo for core count
        std::ifstream cpuinfo("/proc/cpuinfo");
        if (cpuinfo.is_open()) {
            std::string line;
            int cores = 0;
            while (std::getline(cpuinfo, line)) {
                if (line.find("processor") == 0) cores++;
            }
            snap.cpu_cores = cores;
            cpuinfo.close();
        }

        // Read /proc/loadavg
        std::ifstream loadavg("/proc/loadavg");
        if (loadavg.is_open()) {
            loadavg >> snap.load_average_1m >> snap.load_average_5m >> snap.load_average_15m;
            loadavg.close();
            if (snap.cpu_cores > 0) {
                snap.cpu_usage_percent = (snap.load_average_1m / snap.cpu_cores) * 100.0f;
            }
        }

        // Read thermal zone
        std::ifstream thermal("/sys/class/thermal/thermal_zone0/temp");
        if (thermal.is_open()) {
            int millicelsius = 0;
            thermal >> millicelsius;
            snap.temperature_celsius = millicelsius / 1000.0f;
            thermal.close();
        }

        snap.valid = (snap.total_ram_mb > 0 || snap.cpu_cores > 0);
        return snap;
    }

    // Read local system resources (macOS/Darwin)
    MKResourceSnapshot readLocalDarwin() const {
        MKResourceSnapshot snap;
        snap.valid = false;
        snap.timestamp = std::time(nullptr);
        snap.cpu_usage_percent = 0.0f;
        snap.total_ram_mb = 0;
        snap.available_ram_mb = 0;
        snap.total_disk_mb = 0;
        snap.available_disk_mb = 0;
        snap.temperature_celsius = 45.0f;  // Default estimate
        snap.cpu_cores = 0;
        snap.load_average_1m = 0.0f;
        snap.load_average_5m = 0.0f;
        snap.load_average_15m = 0.0f;

        // Get CPU cores
        std::string cores_str = execCmd("sysctl -n hw.ncpu 2>/dev/null");
        if (!cores_str.empty()) {
            try { snap.cpu_cores = std::stoi(cores_str); } catch (...) {}
        }

        // Get total RAM
        std::string mem_str = execCmd("sysctl -n hw.memsize 2>/dev/null");
        if (!mem_str.empty()) {
            try { snap.total_ram_mb = std::stol(mem_str) / (1024 * 1024); } catch (...) {}
        }

        // Estimate available RAM (rough estimate from vm_stat)
        std::string vm_stat = execCmd("vm_stat 2>/dev/null | grep 'Pages free'");
        if (!vm_stat.empty()) {
            size_t colon = vm_stat.find(':');
            if (colon != std::string::npos) {
                std::string val = vm_stat.substr(colon + 1);
                try {
                    long pages = std::stol(val);
                    snap.available_ram_mb = (pages * 4096) / (1024 * 1024);
                } catch (...) {
                    snap.available_ram_mb = snap.total_ram_mb / 2;  // Estimate
                }
            }
        }
        if (snap.available_ram_mb == 0) snap.available_ram_mb = snap.total_ram_mb / 2;

        // Get load average
        std::string uptime_str = execCmd("sysctl -n vm.loadavg 2>/dev/null");
        if (!uptime_str.empty()) {
            // Format: { 1.23 2.34 3.45 }
            std::istringstream iss(uptime_str);
            char brace;
            iss >> brace >> snap.load_average_1m >> snap.load_average_5m >> snap.load_average_15m;
            if (snap.cpu_cores > 0) {
                snap.cpu_usage_percent = (snap.load_average_1m / snap.cpu_cores) * 100.0f;
            }
        }

        snap.valid = (snap.total_ram_mb > 0 || snap.cpu_cores > 0);
        return snap;
    }

public:
    MKResourceMonitor() {
        // Safe default thresholds - the "don't crash" philosophy
        thresholds.max_cpu_percent = 80.0f;
        thresholds.max_ram_percent = 85.0f;
        thresholds.max_temperature = 85.0f;
        thresholds.warning_temperature = 70.0f;
        thresholds.min_free_disk_mb = 1024;  // Keep 1GB free minimum
        thresholds.safety_margin = 0.15f;    // 15% safety margin
    }

    // Get current local resource snapshot
    MKResourceSnapshot getLocalResources() {
        MKResourceSnapshot snap;
        #ifdef __linux__
        snap = readLocalLinux();
        #else
        snap = readLocalDarwin();
        #endif

        device_snapshots["local"] = snap;
        return snap;
    }

    // Store a remote device's resource snapshot
    void updateDeviceResources(const std::string& hostname, const MKResourceSnapshot& snap) {
        device_snapshots[hostname] = snap;
    }

    // The critical function: can a device run a given task safely?
    bool canRunTask(const std::string& hostname, const MKTaskRequirements& requirements) {
        auto it = device_snapshots.find(hostname);
        if (it == device_snapshots.end()) {
            // If we don't have data, we can't guarantee safety - refuse
            overload_log.push_back("REFUSED: No resource data for " + hostname);
            return false;
        }

        const MKResourceSnapshot& snap = it->second;
        if (!snap.valid) {
            overload_log.push_back("REFUSED: Invalid snapshot for " + hostname);
            return false;
        }

        // Check 1: Temperature
        if (snap.temperature_celsius > thresholds.max_temperature) {
            overload_log.push_back("REFUSED: " + hostname + " too hot (" +
                                  std::to_string((int)snap.temperature_celsius) + "C)");
            return false;
        }
        if (requirements.max_temperature > 0 &&
            snap.temperature_celsius > requirements.max_temperature) {
            overload_log.push_back("REFUSED: " + hostname + " exceeds task temp limit");
            return false;
        }

        // Check 2: CPU capacity (with safety margin)
        float available_cpu = thresholds.max_cpu_percent - snap.cpu_usage_percent;
        float needed_with_margin = requirements.cpu_percent_needed * (1.0f + thresholds.safety_margin);
        if (needed_with_margin > available_cpu) {
            overload_log.push_back("REFUSED: " + hostname + " CPU overload (" +
                                  std::to_string((int)snap.cpu_usage_percent) + "% used, need " +
                                  std::to_string((int)requirements.cpu_percent_needed) + "% more)");
            return false;
        }

        // Check 3: RAM capacity (with safety margin)
        long ram_with_margin = static_cast<long>(requirements.ram_mb_needed *
                                                 (1.0f + thresholds.safety_margin));
        if (snap.available_ram_mb < ram_with_margin) {
            overload_log.push_back("REFUSED: " + hostname + " RAM overload (available: " +
                                  std::to_string(snap.available_ram_mb) + "MB, need: " +
                                  std::to_string(requirements.ram_mb_needed) + "MB)");
            return false;
        }

        // Check 4: Disk space
        if (requirements.disk_mb_needed > 0 &&
            snap.available_disk_mb < requirements.disk_mb_needed + thresholds.min_free_disk_mb) {
            overload_log.push_back("REFUSED: " + hostname + " disk space insufficient");
            return false;
        }

        return true;
    }

    // Simplified version: can run task given CPU and RAM requirements only
    bool canRunTask(const std::string& hostname, float cpu_needed, long ram_needed) {
        MKTaskRequirements req;
        req.cpu_percent_needed = cpu_needed;
        req.ram_mb_needed = ram_needed;
        req.disk_mb_needed = 0;
        req.max_temperature = 0;
        return canRunTask(hostname, req);
    }

    // Get current load percentage for a device
    float getDeviceLoad(const std::string& hostname) const {
        auto it = device_snapshots.find(hostname);
        if (it == device_snapshots.end()) return 100.0f;  // Assume full if unknown
        return it->second.cpu_usage_percent;
    }

    // Get temperature for a device
    float getDeviceTemperature(const std::string& hostname) const {
        auto it = device_snapshots.find(hostname);
        if (it == device_snapshots.end()) return 0.0f;
        return it->second.temperature_celsius;
    }

    // Check if a device is in a safe state
    bool isDeviceSafe(const std::string& hostname) const {
        auto it = device_snapshots.find(hostname);
        if (it == device_snapshots.end()) return false;
        const auto& snap = it->second;
        return snap.temperature_celsius < thresholds.warning_temperature &&
               snap.cpu_usage_percent < thresholds.max_cpu_percent;
    }

    // Set custom thresholds
    void setThresholds(const MKResourceThresholds& t) { thresholds = t; }

    // Get overload log
    std::vector<std::string> getOverloadLog() const { return overload_log; }

    // Clear overload log
    void clearOverloadLog() { overload_log.clear(); }

    // Get summary
    std::string getSummary() const {
        std::ostringstream ss;
        ss << "=== Resource Monitor ===\n";
        ss << "  Tracked devices: " << device_snapshots.size() << "\n";
        ss << "  Thresholds: CPU<" << (int)thresholds.max_cpu_percent
           << "%, RAM<" << (int)thresholds.max_ram_percent
           << "%, Temp<" << (int)thresholds.max_temperature << "C\n";
        ss << "  Overload events: " << overload_log.size() << "\n";
        for (const auto& kv : device_snapshots) {
            ss << "  " << kv.first << ": CPU=" << (int)kv.second.cpu_usage_percent
               << "%, RAM_free=" << kv.second.available_ram_mb << "MB"
               << ", Temp=" << (int)kv.second.temperature_celsius << "C\n";
        }
        return ss.str();
    }
};

#endif // MK_RESOURCE_MONITOR_CPP
