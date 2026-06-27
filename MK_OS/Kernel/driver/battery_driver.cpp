#ifndef MK_BATTERY_DRIVER_CPP
#define MK_BATTERY_DRIVER_CPP

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <array>
#include <cstdio>

struct MKBatteryHealth {
    int cycleCount = 0;
    int designCapacity = 0;
    int currentMaxCapacity = 0;
    double healthPercent = 100.0;
    std::string condition = "unknown";
};

class MKBatteryDriver {
private:
    int percentage = -1;
    bool charging = false;
    bool onAC = false;
    int timeRemainingMin = -1;
    double wattsConsumed = 0.0;
    MKBatteryHealth health;

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

    void refresh() {
#ifdef __APPLE__
        refreshMacOS();
#elif defined(__linux__)
        refreshLinux();
#endif
    }

#ifdef __APPLE__
    void refreshMacOS() {
        // Parse pmset -g batt output
        // Example: "Now drawing from 'Battery Power'"
        //          "-InternalBattery-0 (id=...)  85%; discharging; 3:45 remaining"
        std::string output = execCommand("pmset -g batt 2>/dev/null");
        if (output.empty()) return;

        // Detect AC/Battery source
        if (output.find("AC Power") != std::string::npos) {
            onAC = true;
        } else {
            onAC = false;
        }

        // Find percentage
        auto pctPos = output.find('%');
        if (pctPos != std::string::npos) {
            // Walk backwards to find the number
            size_t numStart = pctPos;
            while (numStart > 0 && (output[numStart - 1] >= '0' && output[numStart - 1] <= '9')) {
                numStart--;
            }
            if (numStart < pctPos) {
                try {
                    percentage = std::stoi(output.substr(numStart, pctPos - numStart));
                } catch (...) {}
            }
        }

        // Detect charging state
        charging = (output.find("charging") != std::string::npos &&
                    output.find("discharging") == std::string::npos &&
                    output.find("not charging") == std::string::npos);

        // Time remaining
        auto remainPos = output.find("remaining");
        if (remainPos != std::string::npos) {
            // Find "H:MM remaining" pattern
            size_t searchStart = (remainPos > 10) ? remainPos - 10 : 0;
            std::string beforeRemain = output.substr(searchStart, remainPos - searchStart);
            auto colonPos = beforeRemain.find(':');
            if (colonPos != std::string::npos) {
                size_t hStart = colonPos;
                while (hStart > 0 && beforeRemain[hStart - 1] >= '0' && beforeRemain[hStart - 1] <= '9') {
                    hStart--;
                }
                try {
                    int hours = std::stoi(beforeRemain.substr(hStart, colonPos - hStart));
                    int mins = std::stoi(beforeRemain.substr(colonPos + 1));
                    timeRemainingMin = hours * 60 + mins;
                } catch (...) {}
            }
        }

        // Get cycle count and health from system_profiler
        std::string spOutput = execCommand("system_profiler SPPowerDataType 2>/dev/null");
        if (!spOutput.empty()) {
            std::istringstream stream(spOutput);
            std::string line;
            while (std::getline(stream, line)) {
                if (line.find("Cycle Count") != std::string::npos) {
                    auto colonP = line.find(':');
                    if (colonP != std::string::npos) {
                        try { health.cycleCount = std::stoi(line.substr(colonP + 1)); } catch (...) {}
                    }
                } else if (line.find("Condition") != std::string::npos) {
                    auto colonP = line.find(':');
                    if (colonP != std::string::npos) {
                        health.condition = line.substr(colonP + 2);
                    }
                } else if (line.find("Full Charge Capacity") != std::string::npos) {
                    auto colonP = line.find(':');
                    if (colonP != std::string::npos) {
                        try { health.currentMaxCapacity = std::stoi(line.substr(colonP + 1)); } catch (...) {}
                    }
                } else if (line.find("Wattage") != std::string::npos || line.find("Amperage") != std::string::npos) {
                    auto colonP = line.find(':');
                    if (colonP != std::string::npos) {
                        try { wattsConsumed = std::stod(line.substr(colonP + 1)); } catch (...) {}
                    }
                }
            }
        }
        if (health.designCapacity > 0 && health.currentMaxCapacity > 0) {
            health.healthPercent = (static_cast<double>(health.currentMaxCapacity) /
                                    health.designCapacity) * 100.0;
        }
    }
#endif

#ifdef __linux__
    void refreshLinux() {
        std::string basePath = "/sys/class/power_supply/BAT0/";
        // Try BAT0 first, then BAT1
        std::string testFile = readFileContent(basePath + "present");
        if (testFile.empty()) {
            basePath = "/sys/class/power_supply/BAT1/";
            testFile = readFileContent(basePath + "present");
        }

        if (testFile.empty()) {
            // No battery found
            percentage = -1;
            return;
        }

        std::string capacityStr = readFileContent(basePath + "capacity");
        if (!capacityStr.empty()) {
            try { percentage = std::stoi(capacityStr); } catch (...) {}
        }

        std::string status = readFileContent(basePath + "status");
        charging = (status == "Charging");
        onAC = (status == "Charging" || status == "Full" || status == "Not charging");

        // Energy and power for time remaining
        std::string energyNowStr = readFileContent(basePath + "energy_now");
        std::string powerNowStr = readFileContent(basePath + "power_now");
        if (!energyNowStr.empty() && !powerNowStr.empty()) {
            try {
                long energyNow = std::stol(energyNowStr);   // microWh
                long powerNow = std::stol(powerNowStr);     // microW
                if (powerNow > 0) {
                    wattsConsumed = powerNow / 1000000.0;
                    if (!charging) {
                        double hoursRemaining = static_cast<double>(energyNow) / powerNow;
                        timeRemainingMin = static_cast<int>(hoursRemaining * 60);
                    }
                }
            } catch (...) {}
        }

        // Cycle count
        std::string cycleStr = readFileContent(basePath + "cycle_count");
        if (!cycleStr.empty()) {
            try { health.cycleCount = std::stoi(cycleStr); } catch (...) {}
        }

        // Health from charge_full vs charge_full_design
        std::string chargeFull = readFileContent(basePath + "charge_full");
        std::string chargeDesign = readFileContent(basePath + "charge_full_design");
        if (!chargeFull.empty() && !chargeDesign.empty()) {
            try {
                health.currentMaxCapacity = std::stoi(chargeFull);
                health.designCapacity = std::stoi(chargeDesign);
                if (health.designCapacity > 0) {
                    health.healthPercent = (static_cast<double>(health.currentMaxCapacity) /
                                            health.designCapacity) * 100.0;
                }
            } catch (...) {}
        }

        // Also try energy_full / energy_full_design
        if (health.designCapacity == 0) {
            std::string eFull = readFileContent(basePath + "energy_full");
            std::string eDesign = readFileContent(basePath + "energy_full_design");
            if (!eFull.empty() && !eDesign.empty()) {
                try {
                    health.currentMaxCapacity = std::stoi(eFull);
                    health.designCapacity = std::stoi(eDesign);
                    if (health.designCapacity > 0) {
                        health.healthPercent = (static_cast<double>(health.currentMaxCapacity) /
                                                health.designCapacity) * 100.0;
                    }
                } catch (...) {}
            }
        }

        if (health.healthPercent > 80.0) health.condition = "Good";
        else if (health.healthPercent > 50.0) health.condition = "Fair";
        else health.condition = "Replace Soon";
    }
#endif

public:
    MKBatteryDriver() {
        std::cout << "[DRV:BAT] Initializing battery driver...\n";
        refresh();
        if (percentage >= 0) {
            std::cout << "[DRV:BAT] Battery detected: " << percentage << "%\n";
        } else {
            std::cout << "[DRV:BAT] No battery detected (desktop or unavailable).\n";
        }
    }

    void init() {
        std::cout << "[DRV:BAT] Battery subsystem initialized.\n";
        stats();
    }

    int getPercent() {
        refresh();
        return percentage;
    }

    bool isCharging() {
        refresh();
        return charging;
    }

    bool isOnAC() {
        refresh();
        return onAC;
    }

    int getTimeRemaining() {
        refresh();
        return timeRemainingMin;
    }

    MKBatteryHealth getHealth() {
        refresh();
        return health;
    }

    int getCycleCount() {
        refresh();
        return health.cycleCount;
    }

    double getPowerConsumption() {
        refresh();
        return wattsConsumed;
    }

    bool hasBattery() {
        refresh();
        return (percentage >= 0);
    }

    void stats() const {
        std::cout << "[DRV:BAT] === Battery Statistics ===\n";
        if (percentage < 0) {
            std::cout << "[DRV:BAT] No battery present.\n";
            return;
        }
        std::cout << "[DRV:BAT] Level: " << percentage << "%\n";
        std::cout << "[DRV:BAT] State: " << (charging ? "Charging" : "Discharging") << "\n";
        std::cout << "[DRV:BAT] AC Power: " << (onAC ? "Connected" : "Disconnected") << "\n";
        if (timeRemainingMin > 0) {
            std::cout << "[DRV:BAT] Time Remaining: " << (timeRemainingMin / 60) << "h "
                      << (timeRemainingMin % 60) << "m\n";
        }
        std::cout << "[DRV:BAT] Cycle Count: " << health.cycleCount << "\n";
        std::cout << "[DRV:BAT] Health: " << health.healthPercent << "% ("
                  << health.condition << ")\n";
        if (wattsConsumed > 0) {
            std::cout << "[DRV:BAT] Power Draw: " << wattsConsumed << " W\n";
        }
    }
};

#endif // MK_BATTERY_DRIVER_CPP
