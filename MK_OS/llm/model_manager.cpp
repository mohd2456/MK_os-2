// ============================================================
// MK OS - LLM Model Manager
// Handles hardware detection and RAM availability checks.
// Determines if the system has enough resources to run an LLM.
// ============================================================
#ifndef MK_MODEL_MANAGER_CPP
#define MK_MODEL_MANAGER_CPP

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

class MKModelManager {
private:
    float availableRAM_GB;
    bool hardwareSufficient;
    static constexpr float MIN_RAM_GB = 2.0f;

    // --------------------------------------------------------
    // Read available RAM from /proc/meminfo (Linux)
    // or use sysctl on macOS
    // --------------------------------------------------------
    float readAvailableRAM() {
#ifdef __linux__
        std::ifstream meminfo("/proc/meminfo");
        if (!meminfo.is_open()) {
            return 0.0f;
        }
        std::string line;
        long memAvailableKB = 0;
        long memFreeKB = 0;
        long buffersKB = 0;
        long cachedKB = 0;
        bool foundAvailable = false;

        while (std::getline(meminfo, line)) {
            if (line.find("MemAvailable:") == 0) {
                std::istringstream iss(line.substr(13));
                iss >> memAvailableKB;
                foundAvailable = true;
                break;
            } else if (line.find("MemFree:") == 0) {
                std::istringstream iss(line.substr(8));
                iss >> memFreeKB;
            } else if (line.find("Buffers:") == 0) {
                std::istringstream iss(line.substr(8));
                iss >> buffersKB;
            } else if (line.find("Cached:") == 0) {
                std::istringstream iss(line.substr(7));
                iss >> cachedKB;
            }
        }
        meminfo.close();

        if (foundAvailable) {
            return static_cast<float>(memAvailableKB) / (1024.0f * 1024.0f);
        }
        // Fallback: MemFree + Buffers + Cached
        long totalFreeKB = memFreeKB + buffersKB + cachedKB;
        return static_cast<float>(totalFreeKB) / (1024.0f * 1024.0f);
#elif defined(__APPLE__)
        // macOS: use sysctl to get free memory
        // This is a simplified approach
        FILE* pipe = popen("sysctl -n hw.memsize", "r");
        if (!pipe) return 0.0f;
        char buffer[128];
        std::string result;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        pclose(pipe);
        if (result.empty()) return 0.0f;
        long long totalBytes = std::stoll(result);
        // Estimate ~60% available on a typical system
        return static_cast<float>(totalBytes) * 0.6f / (1024.0f * 1024.0f * 1024.0f);
#else
        return 0.0f;
#endif
    }

public:
    MKModelManager() {
        availableRAM_GB = readAvailableRAM();
        hardwareSufficient = (availableRAM_GB >= MIN_RAM_GB);
        std::cout << "[MODEL MANAGER] Hardware check complete. "
                  << getRAMString() << " available.\n";
    }

    // --------------------------------------------------------
    // Get available RAM in GB
    // --------------------------------------------------------
    float getAvailableRAM() const {
        return availableRAM_GB;
    }

    // --------------------------------------------------------
    // Check if hardware meets minimum requirements
    // --------------------------------------------------------
    bool checkHardware() const {
        return hardwareSufficient;
    }

    // --------------------------------------------------------
    // Get human-readable RAM string
    // --------------------------------------------------------
    std::string getRAMString() const {
        std::ostringstream oss;
        oss.precision(1);
        oss << std::fixed << availableRAM_GB << " GB RAM";
        return oss.str();
    }

    // --------------------------------------------------------
    // Get minimum required RAM
    // --------------------------------------------------------
    float getMinRAM() const {
        return MIN_RAM_GB;
    }
};

#endif // MK_MODEL_MANAGER_CPP
