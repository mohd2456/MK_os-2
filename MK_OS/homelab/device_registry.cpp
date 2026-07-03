#ifndef MK_DEVICE_REGISTRY_CPP
#define MK_DEVICE_REGISTRY_CPP

#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <iostream>

// ===================================================================================
// MK DEVICE REGISTRY
// Maintains a database of all devices MK knows about.
// Each device has: hostname, ip, ssh_user, cpu_model, cpu_cores, ram_mb, gpu_info,
// storage_gb, os_type, last_seen, current_load, temperature, role.
// Persists to JSON file. Auto-discovers devices on LAN.
// ===================================================================================

enum class MKDeviceRole {
    MAIN_RIG,
    COMPUTE_NODE,
    STORAGE,
    PHONE,
    RASPBERRY_PI,
    VM,
    UNKNOWN
};

enum class MKDeviceStatus {
    ONLINE,
    OFFLINE,
    DEGRADED,
    MAINTENANCE,
    UNKNOWN
};

struct MKDevice {
    std::string hostname;
    std::string ip;
    std::string ssh_user;
    std::string cpu_model;
    int cpu_cores;
    long ram_mb;
    std::string gpu_info;
    long storage_gb;
    std::string os_type;
    std::time_t last_seen;
    float current_load;      // CPU load 0-100
    float temperature;       // Celsius
    MKDeviceRole role;
    MKDeviceStatus status;
    int ssh_port;
    std::string ssh_key_path;
    std::map<std::string, std::string> metadata;
};

class MKDeviceRegistry {
private:
    std::map<std::string, MKDevice> devices;  // hostname -> device
    std::string persistence_path;
    std::time_t last_discovery;
    int discovery_port;

    std::string roleToString(MKDeviceRole role) const {
        switch (role) {
            case MKDeviceRole::MAIN_RIG: return "main_rig";
            case MKDeviceRole::COMPUTE_NODE: return "compute_node";
            case MKDeviceRole::STORAGE: return "storage";
            case MKDeviceRole::PHONE: return "phone";
            case MKDeviceRole::RASPBERRY_PI: return "raspberry_pi";
            case MKDeviceRole::VM: return "vm";
            default: return "unknown";
        }
    }

    MKDeviceRole stringToRole(const std::string& s) const {
        if (s == "main_rig") return MKDeviceRole::MAIN_RIG;
        if (s == "compute_node") return MKDeviceRole::COMPUTE_NODE;
        if (s == "storage") return MKDeviceRole::STORAGE;
        if (s == "phone") return MKDeviceRole::PHONE;
        if (s == "raspberry_pi") return MKDeviceRole::RASPBERRY_PI;
        if (s == "vm") return MKDeviceRole::VM;
        return MKDeviceRole::UNKNOWN;
    }

    std::string statusToString(MKDeviceStatus s) const {
        switch (s) {
            case MKDeviceStatus::ONLINE: return "online";
            case MKDeviceStatus::OFFLINE: return "offline";
            case MKDeviceStatus::DEGRADED: return "degraded";
            case MKDeviceStatus::MAINTENANCE: return "maintenance";
            default: return "unknown";
        }
    }

public:
    MKDeviceRegistry() : persistence_path("mk_devices.json"),
                         last_discovery(0), discovery_port(9877) {}

    // Add or update a device
    void addDevice(const MKDevice& device) {
        devices[device.hostname] = device;
        devices[device.hostname].last_seen = std::time(nullptr);
        devices[device.hostname].status = MKDeviceStatus::ONLINE;
    }

    // Remove a device
    bool removeDevice(const std::string& hostname) {
        return devices.erase(hostname) > 0;
    }

    // Get a device by hostname
    MKDevice* getDevice(const std::string& hostname) {
        auto it = devices.find(hostname);
        if (it != devices.end()) return &it->second;
        return nullptr;
    }

    // Get a device by hostname (const)
    const MKDevice* getDevice(const std::string& hostname) const {
        auto it = devices.find(hostname);
        if (it != devices.end()) return &it->second;
        return nullptr;
    }

    // Query devices by role
    std::vector<MKDevice> getDevicesByRole(MKDeviceRole role) const {
        std::vector<MKDevice> result;
        for (const auto& kv : devices) {
            if (kv.second.role == role) result.push_back(kv.second);
        }
        return result;
    }

    // Query devices that are online
    std::vector<MKDevice> getOnlineDevices() const {
        std::vector<MKDevice> result;
        for (const auto& kv : devices) {
            if (kv.second.status == MKDeviceStatus::ONLINE) {
                result.push_back(kv.second);
            }
        }
        return result;
    }

    // Find devices with enough resources
    std::vector<MKDevice> findCapableDevices(int min_cores, long min_ram_mb,
                                             long min_storage_gb = 0) const {
        std::vector<MKDevice> result;
        for (const auto& kv : devices) {
            const auto& d = kv.second;
            if (d.status != MKDeviceStatus::ONLINE) continue;
            if (d.cpu_cores >= min_cores && d.ram_mb >= min_ram_mb) {
                if (min_storage_gb == 0 || d.storage_gb >= min_storage_gb) {
                    result.push_back(d);
                }
            }
        }
        return result;
    }

    // Update device status
    void updateStatus(const std::string& hostname, MKDeviceStatus status) {
        auto it = devices.find(hostname);
        if (it != devices.end()) {
            it->second.status = status;
            if (status == MKDeviceStatus::ONLINE) {
                it->second.last_seen = std::time(nullptr);
            }
        }
    }

    // Update device load
    void updateLoad(const std::string& hostname, float load, float temp) {
        auto it = devices.find(hostname);
        if (it != devices.end()) {
            it->second.current_load = load;
            it->second.temperature = temp;
            it->second.last_seen = std::time(nullptr);
        }
    }

    // Mark stale devices as offline
    void checkStaleDevices(int timeout_seconds = 300) {
        std::time_t now = std::time(nullptr);
        for (auto& kv : devices) {
            if (kv.second.status == MKDeviceStatus::ONLINE) {
                if (std::difftime(now, kv.second.last_seen) > timeout_seconds) {
                    kv.second.status = MKDeviceStatus::OFFLINE;
                }
            }
        }
    }

    // Auto-discover devices on LAN (placeholder - would use UDP broadcast)
    std::vector<std::string> discoverDevices() {
        std::vector<std::string> discovered;
        last_discovery = std::time(nullptr);
        // In a real implementation, this would send UDP broadcast packets
        // and listen for responses from other MK instances
        // For now, return empty (no new devices found)
        return discovered;
    }

    // Get device count
    int deviceCount() const { return static_cast<int>(devices.size()); }

    // Get all hostnames
    std::vector<std::string> getAllHostnames() const {
        std::vector<std::string> names;
        for (const auto& kv : devices) {
            names.push_back(kv.first);
        }
        return names;
    }

    // Persist to JSON-like format
    void save() const {
        std::ofstream out(persistence_path);
        if (!out.is_open()) return;

        out << "{\n  \"devices\": [\n";
        bool first = true;
        for (const auto& kv : devices) {
            if (!first) out << ",\n";
            first = false;
            const auto& d = kv.second;
            out << "    {\n";
            out << "      \"hostname\": \"" << d.hostname << "\",\n";
            out << "      \"ip\": \"" << d.ip << "\",\n";
            out << "      \"ssh_user\": \"" << d.ssh_user << "\",\n";
            out << "      \"cpu_model\": \"" << d.cpu_model << "\",\n";
            out << "      \"cpu_cores\": " << d.cpu_cores << ",\n";
            out << "      \"ram_mb\": " << d.ram_mb << ",\n";
            out << "      \"gpu_info\": \"" << d.gpu_info << "\",\n";
            out << "      \"storage_gb\": " << d.storage_gb << ",\n";
            out << "      \"os_type\": \"" << d.os_type << "\",\n";
            out << "      \"role\": \"" << roleToString(d.role) << "\",\n";
            out << "      \"ssh_port\": " << d.ssh_port << "\n";
            out << "    }";
        }
        out << "\n  ]\n}\n";
        out.close();
    }

    // Load from file
    void load() {
        std::ifstream in(persistence_path);
        if (!in.is_open()) return;
        // Simple line-based parsing (not full JSON parser, but functional)
        std::string content((std::istreambuf_iterator<char>(in)),
                            std::istreambuf_iterator<char>());
        in.close();

        // Parse hostname entries
        size_t pos = 0;
        while ((pos = content.find("\"hostname\":", pos)) != std::string::npos) {
            MKDevice d;
            d.ssh_port = 22;
            d.current_load = 0.0f;
            d.temperature = 0.0f;
            d.status = MKDeviceStatus::UNKNOWN;
            d.last_seen = 0;
            d.cpu_cores = 0;
            d.ram_mb = 0;
            d.storage_gb = 0;
            d.role = MKDeviceRole::UNKNOWN;

            auto extractStr = [&](const std::string& key) -> std::string {
                size_t kpos = content.find("\"" + key + "\":", pos);
                if (kpos == std::string::npos || kpos > pos + 500) return "";
                size_t vstart = content.find("\"", kpos + key.size() + 3);
                if (vstart == std::string::npos) return "";
                size_t vend = content.find("\"", vstart + 1);
                if (vend == std::string::npos) return "";
                return content.substr(vstart + 1, vend - vstart - 1);
            };

            auto extractInt = [&](const std::string& key) -> long {
                size_t kpos = content.find("\"" + key + "\":", pos);
                if (kpos == std::string::npos || kpos > pos + 500) return 0;
                size_t vstart = kpos + key.size() + 3;
                while (vstart < content.size() && !std::isdigit(content[vstart]) && content[vstart] != '-')
                    vstart++;
                size_t vend = vstart;
                if (vend < content.size() && content[vend] == '-') vend++;
                while (vend < content.size() && std::isdigit(content[vend])) vend++;
                if (vend == vstart) return 0;
                try { return std::stol(content.substr(vstart, vend - vstart)); }
                catch (...) { return 0; }
            };

            d.hostname = extractStr("hostname");
            d.ip = extractStr("ip");
            d.ssh_user = extractStr("ssh_user");
            d.cpu_model = extractStr("cpu_model");
            d.cpu_cores = static_cast<int>(extractInt("cpu_cores"));
            d.ram_mb = extractInt("ram_mb");
            d.gpu_info = extractStr("gpu_info");
            d.storage_gb = extractInt("storage_gb");
            d.os_type = extractStr("os_type");
            d.role = stringToRole(extractStr("role"));
            d.ssh_port = static_cast<int>(extractInt("ssh_port"));
            if (d.ssh_port == 0) d.ssh_port = 22;

            if (!d.hostname.empty()) {
                devices[d.hostname] = d;
            }

            pos += 10;
        }
    }

    // Get summary
    std::string getSummary() const {
        std::ostringstream ss;
        ss << "=== Device Registry ===\n";
        ss << "  Total devices: " << devices.size() << "\n";
        int online = 0;
        for (const auto& kv : devices) {
            if (kv.second.status == MKDeviceStatus::ONLINE) online++;
        }
        ss << "  Online: " << online << "/" << devices.size() << "\n";
        for (const auto& kv : devices) {
            const auto& d = kv.second;
            ss << "  " << d.hostname << " [" << statusToString(d.status) << "] "
               << d.ip << " (" << roleToString(d.role) << ") "
               << d.cpu_cores << " cores, " << d.ram_mb << "MB RAM\n";
        }
        return ss.str();
    }
};

#endif // MK_DEVICE_REGISTRY_CPP
