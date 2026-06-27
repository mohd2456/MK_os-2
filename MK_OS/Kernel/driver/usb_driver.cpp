#ifndef MK_USB_DRIVER_CPP
#define MK_USB_DRIVER_CPP

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <array>
#include <cstdio>
#include <map>
#include <algorithm>

struct MKUSBDevice {
    std::string name;
    std::string vendorId;
    std::string productId;
    std::string manufacturer;
    std::string serialNumber;
    std::string usbVersion;  // "2.0", "3.0", etc.
    std::string speed;       // "480M", "5000M" etc.
};

class MKUSBDriver {
private:
    std::vector<MKUSBDevice> deviceList;
    std::vector<MKUSBDevice> previousScan;

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

    void scan() {
        previousScan = deviceList;
        deviceList.clear();
#ifdef __APPLE__
        scanMacOS();
#elif defined(__linux__)
        scanLinux();
#endif
    }

#ifdef __APPLE__
    void scanMacOS() {
        std::string output = execCommand("system_profiler SPUSBDataType 2>/dev/null");
        if (output.empty()) return;

        std::istringstream stream(output);
        std::string line;
        MKUSBDevice currentDev;
        bool inDevice = false;

        while (std::getline(stream, line)) {
            // Device entries are indented names ending with colon
            // Properties are further indented with "Key: Value"

            // Detect a new device name (indented but not a property)
            size_t indent = line.find_first_not_of(' ');
            if (indent == std::string::npos) continue;

            if (indent >= 8 && line.find(':') == line.size() - 1 && line.find("  ") == std::string::npos) {
                // This is likely a device name
                if (inDevice && !currentDev.name.empty()) {
                    deviceList.push_back(currentDev);
                }
                currentDev = MKUSBDevice();
                currentDev.name = line.substr(indent);
                if (!currentDev.name.empty() && currentDev.name.back() == ':') {
                    currentDev.name.pop_back();
                }
                inDevice = true;
            } else if (inDevice) {
                // Parse property
                auto colonPos = line.find(':');
                if (colonPos != std::string::npos && colonPos + 2 < line.size()) {
                    std::string key = line.substr(indent, colonPos - indent);
                    std::string value = line.substr(colonPos + 2);
                    // Trim
                    while (!key.empty() && key.back() == ' ') key.pop_back();
                    while (!value.empty() && value.front() == ' ') value.erase(value.begin());

                    if (key == "Product ID") currentDev.productId = value;
                    else if (key == "Vendor ID") currentDev.vendorId = value;
                    else if (key == "Manufacturer") currentDev.manufacturer = value;
                    else if (key == "Serial Number") currentDev.serialNumber = value;
                    else if (key == "Speed") {
                        currentDev.speed = value;
                        // Infer USB version from speed
                        if (value.find("5") != std::string::npos &&
                            value.find("Gb") != std::string::npos) {
                            currentDev.usbVersion = "3.0";
                        } else if (value.find("480") != std::string::npos) {
                            currentDev.usbVersion = "2.0";
                        } else if (value.find("12") != std::string::npos) {
                            currentDev.usbVersion = "1.1";
                        } else if (value.find("1.5") != std::string::npos) {
                            currentDev.usbVersion = "1.0";
                        } else {
                            currentDev.usbVersion = "2.0";
                        }
                    }
                    else if (key == "Version") currentDev.usbVersion = value;
                }
            }
        }
        // Don't forget last device
        if (inDevice && !currentDev.name.empty()) {
            deviceList.push_back(currentDev);
        }
    }
#endif

#ifdef __linux__
    void scanLinux() {
        // Scan /sys/bus/usb/devices/
        std::string lsOutput = execCommand("ls -d /sys/bus/usb/devices/[0-9]* 2>/dev/null");
        if (lsOutput.empty()) return;

        std::istringstream stream(lsOutput);
        std::string devicePath;
        while (std::getline(stream, devicePath)) {
            // Skip hub entries and root hubs
            std::string devClass = readFileContent(devicePath + "/bDeviceClass");
            if (devClass == "09") continue;  // Hub

            std::string product = readFileContent(devicePath + "/product");
            if (product.empty()) continue;  // No product name, skip

            MKUSBDevice dev;
            dev.name = product;
            dev.vendorId = readFileContent(devicePath + "/idVendor");
            dev.productId = readFileContent(devicePath + "/idProduct");
            dev.manufacturer = readFileContent(devicePath + "/manufacturer");
            dev.serialNumber = readFileContent(devicePath + "/serial");

            std::string speed = readFileContent(devicePath + "/speed");
            dev.speed = speed + " Mbps";
            if (!speed.empty()) {
                int speedVal = 0;
                try { speedVal = std::stoi(speed); } catch (...) {}
                if (speedVal >= 5000) dev.usbVersion = "3.0";
                else if (speedVal >= 480) dev.usbVersion = "2.0";
                else if (speedVal >= 12) dev.usbVersion = "1.1";
                else dev.usbVersion = "1.0";
            }

            std::string version = readFileContent(devicePath + "/version");
            if (!version.empty() && dev.usbVersion.empty()) {
                // Trim and use
                while (!version.empty() && version.front() == ' ') version.erase(version.begin());
                dev.usbVersion = version;
            }

            deviceList.push_back(dev);
        }
    }
#endif

public:
    MKUSBDriver() {
        std::cout << "[DRV:USB] Initializing USB driver...\n";
        scan();
        std::cout << "[DRV:USB] USB driver ready. Found "
                  << deviceList.size() << " device(s).\n";
    }

    void init() {
        std::cout << "[DRV:USB] USB subsystem initialized.\n";
        stats();
    }

    std::vector<MKUSBDevice> listDevices() {
        scan();
        return deviceList;
    }

    std::vector<MKUSBDevice> scanForNew() {
        scan();
        std::vector<MKUSBDevice> newDevices;
        for (const auto& dev : deviceList) {
            bool found = false;
            for (const auto& prev : previousScan) {
                if (dev.vendorId == prev.vendorId &&
                    dev.productId == prev.productId &&
                    dev.serialNumber == prev.serialNumber) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                newDevices.push_back(dev);
            }
        }
        if (!newDevices.empty()) {
            std::cout << "[DRV:USB] Detected " << newDevices.size()
                      << " new device(s).\n";
        }
        return newDevices;
    }

    MKUSBDevice getDeviceInfo(int index) const {
        if (index >= 0 && index < static_cast<int>(deviceList.size())) {
            return deviceList[index];
        }
        return MKUSBDevice();
    }

    int getDeviceCount() const {
        return static_cast<int>(deviceList.size());
    }

    void stats() const {
        std::cout << "[DRV:USB] === USB Statistics ===\n";
        std::cout << "[DRV:USB] Connected Devices: " << deviceList.size() << "\n";
        for (size_t i = 0; i < deviceList.size(); i++) {
            const auto& dev = deviceList[i];
            std::cout << "[DRV:USB]   [" << i << "] " << dev.name;
            if (!dev.manufacturer.empty()) std::cout << " (" << dev.manufacturer << ")";
            std::cout << " | USB " << dev.usbVersion;
            if (!dev.vendorId.empty()) {
                std::cout << " | VID=" << dev.vendorId << " PID=" << dev.productId;
            }
            std::cout << "\n";
        }
    }
};

#endif // MK_USB_DRIVER_CPP
