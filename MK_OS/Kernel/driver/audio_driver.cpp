#ifndef MK_AUDIO_DRIVER_CPP
#define MK_AUDIO_DRIVER_CPP

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <array>
#include <cstdio>

struct MKAudioDevice {
    std::string name;
    std::string type;  // "input" or "output"
    bool isDefault = false;
};

class MKAudioDriver {
private:
    std::vector<MKAudioDevice> devices;
    int currentVolume = -1;
    bool muted = false;
    int sampleRate = 44100;

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

    void scanDevices() {
        devices.clear();
#ifdef __APPLE__
        scanDevicesMacOS();
#elif defined(__linux__)
        scanDevicesLinux();
#endif
    }

#ifdef __APPLE__
    void scanDevicesMacOS() {
        // Use system_profiler for audio devices
        std::string output = execCommand("system_profiler SPAudioDataType 2>/dev/null");
        if (!output.empty()) {
            std::istringstream stream(output);
            std::string line;
            MKAudioDevice currentDev;
            bool inDevice = false;

            while (std::getline(stream, line)) {
                // Device names are indented with specific pattern
                if (line.find("Output:") != std::string::npos) {
                    continue;
                }
                if (line.find("Input:") != std::string::npos) {
                    continue;
                }
                // Look for device entries
                if (line.find("Default Output Device:") != std::string::npos) {
                    auto pos = line.find(':');
                    if (pos != std::string::npos) {
                        std::string val = line.substr(pos + 2);
                        if (val == "Yes") {
                            if (!devices.empty()) devices.back().isDefault = true;
                        }
                    }
                }
                // Simple approach: detect known macOS audio devices
                if (line.find("Built-in") != std::string::npos ||
                    line.find("Speakers") != std::string::npos ||
                    line.find("Microphone") != std::string::npos ||
                    line.find("Headphones") != std::string::npos) {
                    MKAudioDevice dev;
                    // Trim whitespace
                    size_t start = line.find_first_not_of(" \t");
                    if (start != std::string::npos) {
                        std::string name = line.substr(start);
                        // Remove trailing colon if present
                        if (!name.empty() && name.back() == ':') name.pop_back();
                        dev.name = name;
                    }
                    if (line.find("Microphone") != std::string::npos ||
                        line.find("Input") != std::string::npos) {
                        dev.type = "input";
                    } else {
                        dev.type = "output";
                    }
                    devices.push_back(dev);
                }
            }
        }

        // If no devices found, add defaults for MacBook Pro
        if (devices.empty()) {
            devices.push_back({"Built-in Output (Speakers)", "output", true});
            devices.push_back({"Built-in Input (Microphone)", "input", false});
        }

        // Get current volume via osascript
        std::string volStr = execCommand("osascript -e 'output volume of (get volume settings)' 2>/dev/null");
        if (!volStr.empty()) {
            try { currentVolume = std::stoi(volStr); } catch (...) {}
        }

        // Check mute state
        std::string muteStr = execCommand("osascript -e 'output muted of (get volume settings)' 2>/dev/null");
        muted = (muteStr == "true");
    }
#endif

#ifdef __linux__
    void scanDevicesLinux() {
        // Read from /proc/asound
        std::string cards = readFileContent("/proc/asound/cards");
        if (!cards.empty()) {
            std::istringstream stream(cards);
            std::string line;
            while (std::getline(stream, line)) {
                // Format: " 0 [Intel          ]: HDA-Intel - HDA Intel"
                if (line.find('[') != std::string::npos) {
                    auto bracketStart = line.find('[');
                    auto bracketEnd = line.find(']');
                    if (bracketStart != std::string::npos && bracketEnd != std::string::npos) {
                        auto dashPos = line.find(" - ", bracketEnd);
                        std::string name;
                        if (dashPos != std::string::npos) {
                            name = line.substr(dashPos + 3);
                        } else {
                            name = line.substr(bracketStart + 1, bracketEnd - bracketStart - 1);
                        }
                        // Trim
                        while (!name.empty() && (name.front() == ' ' || name.front() == '\t')) {
                            name.erase(name.begin());
                        }
                        while (!name.empty() && (name.back() == ' ' || name.back() == '\t')) {
                            name.pop_back();
                        }
                        if (!name.empty()) {
                            devices.push_back({name + " (Output)", "output", devices.empty()});
                            devices.push_back({name + " (Input)", "input", false});
                        }
                    }
                }
            }
        }

        // Get volume from amixer
        std::string amixerOut = execCommand("amixer get Master 2>/dev/null");
        if (!amixerOut.empty()) {
            // Look for percentage in brackets: [75%]
            auto bracketPos = amixerOut.find('[');
            while (bracketPos != std::string::npos) {
                auto endBracket = amixerOut.find(']', bracketPos);
                if (endBracket != std::string::npos) {
                    std::string val = amixerOut.substr(bracketPos + 1, endBracket - bracketPos - 1);
                    if (val.back() == '%') {
                        try { currentVolume = std::stoi(val); } catch (...) {}
                        break;
                    }
                    if (val == "off") {
                        muted = true;
                    }
                }
                bracketPos = amixerOut.find('[', bracketPos + 1);
            }
        }
    }
#endif

public:
    MKAudioDriver() {
        std::cout << "[DRV:AUDIO] Initializing audio driver...\n";
        scanDevices();
        std::cout << "[DRV:AUDIO] Audio driver ready. Found "
                  << devices.size() << " device(s).\n";
    }

    void init() {
        std::cout << "[DRV:AUDIO] Audio subsystem initialized.\n";
        stats();
    }

    std::vector<MKAudioDevice> listDevices() {
        scanDevices();
        return devices;
    }

    int getVolume() const { return currentVolume; }

    bool setVolume(int level) {
        if (level < 0 || level > 100) {
            std::cout << "[DRV:AUDIO] ERROR: Volume must be 0-100.\n";
            return false;
        }
#ifdef __APPLE__
        std::string cmd = "osascript -e 'set volume output volume " +
                          std::to_string(level) + "' 2>/dev/null";
        execCommand(cmd);
        currentVolume = level;
        std::cout << "[DRV:AUDIO] Volume set to " << level << "%\n";
        return true;
#elif defined(__linux__)
        std::string cmd = "amixer set Master " + std::to_string(level) + "% 2>/dev/null";
        execCommand(cmd);
        currentVolume = level;
        std::cout << "[DRV:AUDIO] Volume set to " << level << "%\n";
        return true;
#else
        return false;
#endif
    }

    bool isMuted() const { return muted; }

    bool toggleMute() {
#ifdef __APPLE__
        muted = !muted;
        std::string state = muted ? "true" : "false";
        std::string cmd = "osascript -e 'set volume output muted " + state + "' 2>/dev/null";
        execCommand(cmd);
        std::cout << "[DRV:AUDIO] Mute: " << (muted ? "ON" : "OFF") << "\n";
        return true;
#elif defined(__linux__)
        execCommand("amixer set Master toggle 2>/dev/null");
        muted = !muted;
        std::cout << "[DRV:AUDIO] Mute: " << (muted ? "ON" : "OFF") << "\n";
        return true;
#else
        return false;
#endif
    }

    int getSampleRate() const {
#ifdef __APPLE__
        // Default macOS sample rate for built-in audio
        return 44100;
#elif defined(__linux__)
        std::string content = readFileContent("/proc/asound/card0/pcm0p/sub0/hw_params");
        if (!content.empty()) {
            std::istringstream stream(content);
            std::string line;
            while (std::getline(stream, line)) {
                if (line.find("rate:") != std::string::npos) {
                    auto pos = line.find(':');
                    if (pos != std::string::npos) {
                        try { return std::stoi(line.substr(pos + 2)); } catch (...) {}
                    }
                }
            }
        }
        return 44100;
#else
        return 44100;
#endif
    }

    void stats() const {
        std::cout << "[DRV:AUDIO] === Audio Statistics ===\n";
        std::cout << "[DRV:AUDIO] Devices: " << devices.size() << "\n";
        for (const auto& dev : devices) {
            std::cout << "[DRV:AUDIO]   " << dev.type << ": " << dev.name
                      << (dev.isDefault ? " (default)" : "") << "\n";
        }
        std::cout << "[DRV:AUDIO] Volume: " << currentVolume << "%\n";
        std::cout << "[DRV:AUDIO] Muted: " << (muted ? "Yes" : "No") << "\n";
        std::cout << "[DRV:AUDIO] Sample Rate: " << getSampleRate() << " Hz\n";
    }
};

#endif // MK_AUDIO_DRIVER_CPP
