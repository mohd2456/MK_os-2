#ifndef MK_DISPLAY_DRIVER_CPP
#define MK_DISPLAY_DRIVER_CPP

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <array>
#include <cstdio>
#include <vector>

struct MKDisplayInfo {
    int width = 0;
    int height = 0;
    int colorDepth = 32;
    int refreshRate = 60;
    std::string name = "Unknown";
};

class MKDisplayDriver {
private:
    std::vector<MKDisplayInfo> displays;
    int brightness = -1;  // 0-100, -1 if unknown

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

    void detectDisplays() {
        displays.clear();
#ifdef __APPLE__
        detectDisplaysMacOS();
#elif defined(__linux__)
        detectDisplaysLinux();
#endif
        if (displays.empty()) {
            MKDisplayInfo defaultDisplay;
            defaultDisplay.name = "Unknown Display";
            displays.push_back(defaultDisplay);
        }
    }

#ifdef __APPLE__
    void detectDisplaysMacOS() {
        std::string output = execCommand("system_profiler SPDisplaysDataType 2>/dev/null");
        if (output.empty()) return;

        std::istringstream stream(output);
        std::string line;
        MKDisplayInfo currentDisplay;
        bool inDisplay = false;

        while (std::getline(stream, line)) {
            if (line.find("Display Type") != std::string::npos ||
                line.find("Color LCD") != std::string::npos) {
                if (inDisplay && currentDisplay.width > 0) {
                    displays.push_back(currentDisplay);
                }
                currentDisplay = MKDisplayInfo();
                size_t start = line.find_first_not_of(' ');
                if (start != std::string::npos) {
                    std::string name = line.substr(start);
                    if (!name.empty() && name.back() == ':') name.pop_back();
                    currentDisplay.name = name;
                }
                inDisplay = true;
            } else if (line.find("Resolution:") != std::string::npos) {
                auto pos = line.find(':');
                if (pos != std::string::npos) {
                    std::string val = line.substr(pos + 2);
                    auto xPos = val.find(" x ");
                    if (xPos != std::string::npos) {
                        try {
                            currentDisplay.width = std::stoi(val.substr(0, xPos));
                            std::string rest = val.substr(xPos + 3);
                            std::istringstream restStream(rest);
                            restStream >> currentDisplay.height;
                        } catch (...) {}
                    }
                }
            } else if (line.find("Pixel Depth") != std::string::npos ||
                       line.find("Depth:") != std::string::npos) {
                auto pos = line.find(':');
                if (pos != std::string::npos) {
                    std::string val = line.substr(pos + 2);
                    // Parse "32-Bit Color" or just number
                    try { currentDisplay.colorDepth = std::stoi(val); } catch (...) {}
                }
            } else if (line.find("UI Looks like") != std::string::npos) {
                auto pos = line.find(':');
                if (pos != std::string::npos) {
                    std::string val = line.substr(pos + 2);
                    auto xPos = val.find(" x ");
                    if (xPos != std::string::npos) {
                        try {
                            currentDisplay.width = std::stoi(val.substr(0, xPos));
                            std::istringstream restStream(val.substr(xPos + 3));
                            restStream >> currentDisplay.height;
                        } catch (...) {}
                    }
                }
            }
        }
        if (inDisplay && currentDisplay.width > 0) {
            displays.push_back(currentDisplay);
        }

        // Get brightness via AppleScript or ioreg
        std::string brightnessStr = execCommand(
            "ioreg -c AppleBacklightDisplay -r -d 1 2>/dev/null | grep brightness");
        if (!brightnessStr.empty()) {
            // Look for numeric value
            auto eqPos = brightnessStr.find('=');
            if (eqPos != std::string::npos) {
                std::string val = brightnessStr.substr(eqPos + 1);
                while (!val.empty() && val.front() == ' ') val.erase(val.begin());
                try {
                    int rawBrightness = std::stoi(val);
                    // Normalize to 0-100
                    brightness = (rawBrightness * 100) / 1024;
                    if (brightness > 100) brightness = 100;
                } catch (...) {}
            }
        }

        // Default to standard MacBook Pro refresh rate
        for (auto& disp : displays) {
            if (disp.refreshRate == 0) disp.refreshRate = 60;
        }
    }
#endif

#ifdef __linux__
    void detectDisplaysLinux() {
        // Try xrandr
        std::string output = execCommand("xrandr --current 2>/dev/null");
        if (!output.empty()) {
            std::istringstream stream(output);
            std::string line;
            while (std::getline(stream, line)) {
                // Connected display lines: "HDMI-1 connected 1920x1080+0+0"
                if (line.find(" connected") != std::string::npos) {
                    MKDisplayInfo disp;
                    std::istringstream lineStream(line);
                    lineStream >> disp.name;  // Interface name

                    // Look for resolution in the line
                    auto resPos = line.find_first_of("0123456789", line.find("connected") + 9);
                    if (resPos != std::string::npos) {
                        std::string resStr = line.substr(resPos);
                        auto xPos = resStr.find('x');
                        if (xPos != std::string::npos) {
                            try {
                                disp.width = std::stoi(resStr.substr(0, xPos));
                                disp.height = std::stoi(resStr.substr(xPos + 1));
                            } catch (...) {}
                        }
                    }
                    displays.push_back(disp);
                }
                // Active mode line with asterisk
                if (line.find('*') != std::string::npos && !displays.empty()) {
                    std::istringstream modeStream(line);
                    std::string res;
                    double rate;
                    modeStream >> res >> rate;
                    if (rate > 0) {
                        displays.back().refreshRate = static_cast<int>(rate);
                    }
                    if (displays.back().width == 0) {
                        auto xPos = res.find('x');
                        if (xPos != std::string::npos) {
                            try {
                                displays.back().width = std::stoi(res.substr(0, xPos));
                                displays.back().height = std::stoi(res.substr(xPos + 1));
                            } catch (...) {}
                        }
                    }
                }
            }
        }

        // Get brightness from /sys/class/backlight
        std::string blPath = "";
        std::string lsBacklight = execCommand("ls /sys/class/backlight/ 2>/dev/null | head -1");
        if (!lsBacklight.empty()) {
            blPath = "/sys/class/backlight/" + lsBacklight + "/";
            std::string curBright = readFileContent(blPath + "brightness");
            std::string maxBright = readFileContent(blPath + "max_brightness");
            if (!curBright.empty() && !maxBright.empty()) {
                try {
                    int cur = std::stoi(curBright);
                    int mx = std::stoi(maxBright);
                    if (mx > 0) brightness = (cur * 100) / mx;
                } catch (...) {}
            }
        }
    }
#endif

public:
    MKDisplayDriver() {
        std::cout << "[DRV:DISP] Initializing display driver...\n";
        detectDisplays();
        std::cout << "[DRV:DISP] Display driver ready. Found "
                  << displays.size() << " display(s).\n";
    }

    void init() {
        std::cout << "[DRV:DISP] Display subsystem initialized.\n";
        stats();
    }

    MKDisplayInfo getResolution(int displayIndex = 0) const {
        if (displayIndex >= 0 && displayIndex < static_cast<int>(displays.size())) {
            return displays[displayIndex];
        }
        return MKDisplayInfo();
    }

    int getBrightness() const { return brightness; }

    bool setBrightness(int level) {
        if (level < 0 || level > 100) {
            std::cout << "[DRV:DISP] ERROR: Brightness must be 0-100.\n";
            return false;
        }
#ifdef __APPLE__
        // Use brightness command or ioreg (requires appropriate permissions)
        double fraction = level / 100.0;
        std::string cmd = "brightness " + std::to_string(fraction) + " 2>/dev/null";
        execCommand(cmd);
        brightness = level;
        std::cout << "[DRV:DISP] Brightness set to " << level << "%\n";
        return true;
#elif defined(__linux__)
        std::string lsBacklight = execCommand("ls /sys/class/backlight/ 2>/dev/null | head -1");
        if (!lsBacklight.empty()) {
            std::string blPath = "/sys/class/backlight/" + lsBacklight + "/";
            std::string maxStr = readFileContent(blPath + "max_brightness");
            if (!maxStr.empty()) {
                try {
                    int maxVal = std::stoi(maxStr);
                    int targetVal = (level * maxVal) / 100;
                    std::string cmd = "echo " + std::to_string(targetVal) +
                                      " > " + blPath + "brightness 2>/dev/null";
                    execCommand(cmd);
                    brightness = level;
                    std::cout << "[DRV:DISP] Brightness set to " << level << "%\n";
                    return true;
                } catch (...) {}
            }
        }
        return false;
#else
        return false;
#endif
    }

    int getDisplayCount() const {
        return static_cast<int>(displays.size());
    }

    int getColorDepth(int displayIndex = 0) const {
        if (displayIndex >= 0 && displayIndex < static_cast<int>(displays.size())) {
            return displays[displayIndex].colorDepth;
        }
        return 32;
    }

    int getRefreshRate(int displayIndex = 0) const {
        if (displayIndex >= 0 && displayIndex < static_cast<int>(displays.size())) {
            return displays[displayIndex].refreshRate;
        }
        return 60;
    }

    void stats() const {
        std::cout << "[DRV:DISP] === Display Statistics ===\n";
        std::cout << "[DRV:DISP] Display Count: " << displays.size() << "\n";
        for (size_t i = 0; i < displays.size(); i++) {
            const auto& d = displays[i];
            std::cout << "[DRV:DISP]   [" << i << "] " << d.name
                      << " | " << d.width << "x" << d.height
                      << " @ " << d.refreshRate << "Hz"
                      << " | " << d.colorDepth << "-bit color\n";
        }
        if (brightness >= 0) {
            std::cout << "[DRV:DISP] Brightness: " << brightness << "%\n";
        }
    }
};

#endif // MK_DISPLAY_DRIVER_CPP
