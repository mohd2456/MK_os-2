#ifndef MK_GPU_DRIVER_CPP
#define MK_GPU_DRIVER_CPP

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <array>
#include <cstdio>
#include <vector>

struct MKDisplayResolution {
    int width = 0;
    int height = 0;
    int refreshRate = 0;
};

struct MKGPUInfo {
    std::string model = "Unknown";
    long vramMB = 0;
    double temperature = -1.0;
    MKDisplayResolution resolution;
};

class MKGPUDriver {
private:
    MKGPUInfo gpuInfo;

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

    void detectGPU() {
#ifdef __APPLE__
        detectGPUMacOS();
#elif defined(__linux__)
        detectGPULinux();
#endif
    }

#ifdef __APPLE__
    void detectGPUMacOS() {
        std::string output = execCommand("system_profiler SPDisplaysDataType 2>/dev/null");
        if (output.empty()) return;

        std::istringstream stream(output);
        std::string line;
        while (std::getline(stream, line)) {
            // GPU model - look for "Chipset Model:"
            if (line.find("Chipset Model:") != std::string::npos) {
                auto pos = line.find(':');
                if (pos != std::string::npos) {
                    gpuInfo.model = line.substr(pos + 2);
                }
            }
            // VRAM
            else if (line.find("VRAM") != std::string::npos ||
                     line.find("Memory") != std::string::npos) {
                auto pos = line.find(':');
                if (pos != std::string::npos) {
                    std::string val = line.substr(pos + 2);
                    // Parse "256 MB" or "1024 MB" or "1 GB"
                    std::istringstream valStream(val);
                    long amount = 0;
                    std::string unit;
                    valStream >> amount >> unit;
                    if (unit == "GB") amount *= 1024;
                    if (amount > 0) gpuInfo.vramMB = amount;
                }
            }
            // Resolution
            else if (line.find("Resolution:") != std::string::npos) {
                auto pos = line.find(':');
                if (pos != std::string::npos) {
                    std::string val = line.substr(pos + 2);
                    // Parse "1440 x 900" or similar
                    auto xPos = val.find(" x ");
                    if (xPos != std::string::npos) {
                        try {
                            gpuInfo.resolution.width = std::stoi(val.substr(0, xPos));
                            std::string rest = val.substr(xPos + 3);
                            // Might have extra info after resolution
                            std::istringstream restStream(rest);
                            restStream >> gpuInfo.resolution.height;
                        } catch (...) {}
                    }
                }
            }
            // Refresh rate - look for "UI Looks like:" or display refresh
            else if (line.find("Refresh Rate") != std::string::npos ||
                     line.find("Framebuffer") != std::string::npos) {
                auto pos = line.find(':');
                if (pos != std::string::npos) {
                    try {
                        gpuInfo.resolution.refreshRate = std::stoi(line.substr(pos + 2));
                    } catch (...) {}
                }
            }
        }

        // Default refresh rate for 2010 MacBook Pro
        if (gpuInfo.resolution.refreshRate == 0) {
            gpuInfo.resolution.refreshRate = 60;
        }
    }
#endif

#ifdef __linux__
    void detectGPULinux() {
        // Try lspci for GPU model
        std::string lspci = execCommand("lspci 2>/dev/null | grep -i 'vga\\|3d\\|display'");
        if (!lspci.empty()) {
            // Format: "xx:xx.x VGA compatible controller: NVIDIA..."
            auto pos = lspci.find(':');
            if (pos != std::string::npos) {
                pos = lspci.find(':', pos + 1);
                if (pos != std::string::npos) {
                    gpuInfo.model = lspci.substr(pos + 2);
                }
            }
        }

        // Try /sys/class/drm for info
        std::string cardPath = "/sys/class/drm/card0/";
        std::string devicePath = cardPath + "device/";

        // Get VRAM from device/mem_info_vram_total (AMD) or resource
        std::string vram = readFileContent(devicePath + "mem_info_vram_total");
        if (!vram.empty()) {
            try { gpuInfo.vramMB = std::stol(vram) / (1024 * 1024); } catch (...) {}
        }

        // Temperature from hwmon
        std::string temp = execCommand("cat /sys/class/drm/card0/device/hwmon/hwmon*/temp1_input 2>/dev/null");
        if (!temp.empty()) {
            try { gpuInfo.temperature = std::stol(temp) / 1000.0; } catch (...) {}
        }

        // Resolution from xrandr
        std::string xrandr = execCommand("xrandr --current 2>/dev/null | grep '*'");
        if (!xrandr.empty()) {
            // Format: "   1920x1080     60.00*+"
            std::istringstream iss(xrandr);
            std::string res;
            iss >> res;
            auto xPos = res.find('x');
            if (xPos != std::string::npos) {
                try {
                    gpuInfo.resolution.width = std::stoi(res.substr(0, xPos));
                    gpuInfo.resolution.height = std::stoi(res.substr(xPos + 1));
                } catch (...) {}
            }
            double rate;
            if (iss >> rate) {
                gpuInfo.resolution.refreshRate = static_cast<int>(rate);
            }
        }
    }
#endif

public:
    MKGPUDriver() {
        std::cout << "[DRV:GPU] Initializing GPU driver...\n";
        detectGPU();
        std::cout << "[DRV:GPU] GPU driver ready: " << gpuInfo.model << "\n";
    }

    void init() {
        std::cout << "[DRV:GPU] GPU subsystem initialized.\n";
        stats();
    }

    std::string getModel() const { return gpuInfo.model; }

    long getVRAM() const { return gpuInfo.vramMB; }

    double getTemperature() const {
#ifdef __APPLE__
        // macOS GPU temperature typically requires IOKit/SMC access
        return -1.0;
#elif defined(__linux__)
        std::string temp = execCommand("cat /sys/class/drm/card0/device/hwmon/hwmon*/temp1_input 2>/dev/null");
        if (!temp.empty()) {
            try { return std::stol(temp) / 1000.0; } catch (...) {}
        }
        return -1.0;
#else
        return -1.0;
#endif
    }

    MKDisplayResolution getResolution() const { return gpuInfo.resolution; }

    int getRefreshRate() const { return gpuInfo.resolution.refreshRate; }

    MKGPUInfo getFullInfo() const { return gpuInfo; }

    void refresh() {
        detectGPU();
    }

    void stats() const {
        std::cout << "[DRV:GPU] === GPU Statistics ===\n";
        std::cout << "[DRV:GPU] Model: " << gpuInfo.model << "\n";
        std::cout << "[DRV:GPU] VRAM: " << gpuInfo.vramMB << " MB\n";
        if (gpuInfo.resolution.width > 0) {
            std::cout << "[DRV:GPU] Resolution: " << gpuInfo.resolution.width
                      << "x" << gpuInfo.resolution.height << "\n";
            std::cout << "[DRV:GPU] Refresh Rate: " << gpuInfo.resolution.refreshRate << " Hz\n";
        }
        double temp = getTemperature();
        if (temp >= 0) {
            std::cout << "[DRV:GPU] Temperature: " << temp << " C\n";
        }
    }
};

#endif // MK_GPU_DRIVER_CPP
