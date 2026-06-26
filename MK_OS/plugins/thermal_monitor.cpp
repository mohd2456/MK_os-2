#ifndef MK_THERMAL_MONITOR_CPP
#define MK_THERMAL_MONITOR_CPP

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <ctime>
#include <algorithm>
#include <numeric>

// ===================================================================================
// MK THERMAL & HARDWARE MONITOR
// Tracks CPU temperature, fan speed, battery health, and system load.
// On legacy hardware (2010 MacBook), prevents thermal throttling by dynamically
// scaling MK's processing intensity. Triggers power-saver mode on battery.
// ===================================================================================

struct MKThermalReading {
    std::time_t timestamp;
    float cpuTempC;
    int fanSpeedRPM;
    float batteryPercent;
    bool onACPower;
    float cpuLoadPercent;
    float memUsedPercent;
};

enum class MKThermalState {
    COOL,           // <50°C — full performance
    WARM,           // 50-70°C — normal operation
    HOT,            // 70-85°C — begin throttling
    CRITICAL        // >85°C — emergency power reduction
};

class MKThermalMonitor {
private:
    std::vector<MKThermalReading> history;
    MKThermalState currentState;
    int maxHistorySize;
    
    // Thermal thresholds (Celsius)
    float warmThreshold;
    float hotThreshold;
    float criticalThreshold;
    
    // Read CPU temperature from Linux sysfs
    float readCPUTemp() {
        // Try multiple thermal zone paths
        std::vector<std::string> paths = {
            "/sys/class/thermal/thermal_zone0/temp",
            "/sys/class/thermal/thermal_zone1/temp",
            "/sys/class/hwmon/hwmon0/temp1_input"
        };
        
        for (const auto& path : paths) {
            std::ifstream file(path);
            if (file.is_open()) {
                int milliTemp;
                file >> milliTemp;
                file.close();
                return milliTemp / 1000.0f;  // Convert millidegrees to degrees
            }
        }
        return -1.0f;  // Cannot read temperature
    }
    
    // Read fan speed
    int readFanSpeed() {
        std::ifstream file("/sys/class/hwmon/hwmon0/fan1_input");
        if (file.is_open()) {
            int rpm;
            file >> rpm;
            file.close();
            return rpm;
        }
        return -1;
    }
    
    // Read battery status
    float readBatteryPercent() {
        std::ifstream file("/sys/class/power_supply/BAT0/capacity");
        if (!file.is_open()) {
            file.open("/sys/class/power_supply/BAT1/capacity");
        }
        if (file.is_open()) {
            float percent;
            file >> percent;
            file.close();
            return percent;
        }
        return -1.0f;
    }
    
    bool readACStatus() {
        std::ifstream file("/sys/class/power_supply/AC/online");
        if (file.is_open()) {
            int online;
            file >> online;
            file.close();
            return online == 1;
        }
        return true;  // Assume AC if can't detect
    }
    
    // Read CPU load from /proc/stat
    float readCPULoad() {
        std::ifstream file("/proc/stat");
        if (!file.is_open()) return -1.0f;
        
        std::string line;
        std::getline(file, line);
        file.close();
        
        std::stringstream ss(line);
        std::string cpu;
        long user, nice, system, idle, iowait, irq, softirq;
        ss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq;
        
        long total = user + nice + system + idle + iowait + irq + softirq;
        long active = total - idle - iowait;
        
        return (total > 0) ? ((float)active / total * 100.0f) : 0.0f;
    }
    
    // Read memory usage
    float readMemUsage() {
        std::ifstream file("/proc/meminfo");
        if (!file.is_open()) return -1.0f;
        
        long total = 0, available = 0;
        std::string line;
        while (std::getline(file, line)) {
            if (line.find("MemTotal:") == 0) {
                std::sscanf(line.c_str(), "MemTotal: %ld", &total);
            } else if (line.find("MemAvailable:") == 0) {
                std::sscanf(line.c_str(), "MemAvailable: %ld", &available);
            }
        }
        file.close();
        
        if (total > 0) return ((float)(total - available) / total) * 100.0f;
        return -1.0f;
    }
    
    MKThermalState classifyTemp(float tempC) {
        if (tempC < 0) return MKThermalState::COOL;  // Can't read
        if (tempC >= criticalThreshold) return MKThermalState::CRITICAL;
        if (tempC >= hotThreshold) return MKThermalState::HOT;
        if (tempC >= warmThreshold) return MKThermalState::WARM;
        return MKThermalState::COOL;
    }

public:
    MKThermalMonitor() : currentState(MKThermalState::COOL), maxHistorySize(1000),
                         warmThreshold(50.0f), hotThreshold(70.0f), criticalThreshold(85.0f) {
        std::cout << "[THERMAL] Hardware monitor initialized.\n";
    }
    
    // Take a snapshot of current hardware state
    MKThermalReading sample() {
        MKThermalReading reading;
        reading.timestamp = std::time(nullptr);
        reading.cpuTempC = readCPUTemp();
        reading.fanSpeedRPM = readFanSpeed();
        reading.batteryPercent = readBatteryPercent();
        reading.onACPower = readACStatus();
        reading.cpuLoadPercent = readCPULoad();
        reading.memUsedPercent = readMemUsage();
        
        currentState = classifyTemp(reading.cpuTempC);
        
        history.push_back(reading);
        if ((int)history.size() > maxHistorySize) {
            history.erase(history.begin());
        }
        
        return reading;
    }
    
    // Get recommended performance scaling factor (0.0 to 1.0)
    float getPerformanceScale() {
        switch (currentState) {
            case MKThermalState::COOL:     return 1.0f;    // Full speed
            case MKThermalState::WARM:     return 0.85f;   // Slight reduction
            case MKThermalState::HOT:      return 0.5f;    // Half speed
            case MKThermalState::CRITICAL: return 0.2f;    // Emergency minimum
        }
        return 1.0f;
    }
    
    // Should heavy background tasks run now?
    bool canRunHeavyTasks() {
        if (currentState >= MKThermalState::HOT) return false;
        if (!history.empty() && !history.back().onACPower && 
            history.back().batteryPercent < 20.0f) return false;
        return true;
    }
    
    // Should opportunistic tasks run? (model optimization, indexing)
    bool isIdleAndPowered() {
        if (history.empty()) return false;
        const auto& last = history.back();
        return last.onACPower && last.cpuLoadPercent < 30.0f && 
               currentState <= MKThermalState::WARM;
    }
    
    std::string stateLabel() const {
        switch (currentState) {
            case MKThermalState::COOL:     return "COOL";
            case MKThermalState::WARM:     return "WARM";
            case MKThermalState::HOT:      return "HOT";
            case MKThermalState::CRITICAL: return "CRITICAL";
        }
        return "UNKNOWN";
    }
    
    // Get average temperature over last N samples
    float getAverageTemp(int samples = 10) {
        if (history.empty()) return 0.0f;
        int count = std::min(samples, (int)history.size());
        float sum = 0.0f;
        for (int i = history.size() - count; i < (int)history.size(); i++) {
            sum += history[i].cpuTempC;
        }
        return sum / count;
    }
    
    void printStatus() {
        auto reading = sample();
        std::cout << "\n--- [THERMAL STATUS] ---\n";
        std::cout << "  CPU Temp: " << reading.cpuTempC << "°C (" << stateLabel() << ")\n";
        std::cout << "  Fan: " << reading.fanSpeedRPM << " RPM\n";
        std::cout << "  Battery: " << reading.batteryPercent << "% " 
                  << (reading.onACPower ? "[AC]" : "[BATTERY]") << "\n";
        std::cout << "  CPU Load: " << reading.cpuLoadPercent << "%\n";
        std::cout << "  Memory: " << reading.memUsedPercent << "% used\n";
        std::cout << "  Performance Scale: " << (getPerformanceScale() * 100) << "%\n";
        std::cout << "------------------------\n";
    }
};

#endif // MK_THERMAL_MONITOR_CPP
