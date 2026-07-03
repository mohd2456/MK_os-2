#ifndef MK_DEVICE_COMM_CPP
#define MK_DEVICE_COMM_CPP

#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <sstream>
#include <functional>
#include <algorithm>

// ===================================================================================
// MK DEVICE COMM
// Communication between MK instances using simple HTTP REST API on each device.
// Endpoints: /api/sync (push/pull facts), /api/status (device health),
// /api/task (delegate a task to another device).
// Includes authentication via shared secret.
// ===================================================================================

enum class MKCommMethod {
    HTTP,
    SSH,
    LOCAL  // Same device (in-memory)
};

struct MKCommEndpoint {
    std::string device_id;
    std::string base_url;      // e.g., "http://192.168.1.100:8765"
    MKCommMethod method;
    std::string shared_secret;
    bool authenticated;
    std::time_t last_contact;
    int failed_attempts;
    int max_failures;
};

struct MKCommMessage {
    std::string from_device;
    std::string to_device;
    std::string endpoint;      // /api/sync, /api/status, /api/task
    std::string method;        // GET, POST
    std::string body;
    std::string auth_token;
    std::time_t sent_at;
    int timeout_ms;
};

struct MKCommResponse {
    int status_code;
    std::string body;
    float latency_ms;
    bool success;
    std::string error;
};

struct MKTaskDelegation {
    int task_id;
    std::string from_device;
    std::string to_device;
    std::string command;
    std::string parameters;
    std::time_t delegated_at;
    std::time_t completed_at;
    std::string result;
    bool completed;
    bool failed;
};

struct MKDeviceHealthStatus {
    std::string device_id;
    float cpu_percent;
    long ram_available_mb;
    float temperature;
    int active_tasks;
    std::time_t uptime_seconds;
    std::string version;
    bool accepting_tasks;
};

class MKDeviceComm {
private:
    std::string local_device_id;
    std::string local_secret;
    int listen_port;
    std::map<std::string, MKCommEndpoint> endpoints;
    std::vector<MKTaskDelegation> delegated_tasks;
    std::vector<MKCommMessage> message_log;
    int next_task_id;
    int max_log_size;

    // Generate auth token from shared secret + timestamp
    std::string generateAuthToken(const std::string& secret, std::time_t timestamp) const {
        // Simple HMAC-like token: hash(secret + timestamp)
        // In production, use proper HMAC-SHA256
        unsigned long hash = 5381;
        std::string input = secret + std::to_string(timestamp);
        for (char c : input) {
            hash = ((hash << 5) + hash) + static_cast<unsigned long>(c);
        }
        std::ostringstream ss;
        ss << std::hex << hash;
        return ss.str();
    }

    // Verify auth token
    bool verifyAuthToken(const std::string& token, const std::string& secret) const {
        // Verify against recent timestamps (within 5 min window)
        std::time_t now = std::time(nullptr);
        for (int offset = -300; offset <= 300; offset += 60) {
            std::string expected = generateAuthToken(secret, now + offset);
            if (token == expected) return true;
        }
        return false;
    }

    void trimLog() {
        if ((int)message_log.size() > max_log_size) {
            message_log.erase(message_log.begin(),
                            message_log.begin() + ((int)message_log.size() - max_log_size));
        }
    }

public:
    MKDeviceComm() : local_device_id("mk_local"), local_secret("mk_shared_secret_change_me"),
                     listen_port(8765), next_task_id(1), max_log_size(500) {}

    // Set local device identity
    void setLocalDevice(const std::string& id, const std::string& secret, int port = 8765) {
        local_device_id = id;
        local_secret = secret;
        listen_port = port;
    }

    // Register a remote endpoint
    void registerEndpoint(const std::string& device_id, const std::string& base_url,
                         const std::string& secret, MKCommMethod method = MKCommMethod::HTTP) {
        MKCommEndpoint ep;
        ep.device_id = device_id;
        ep.base_url = base_url;
        ep.method = method;
        ep.shared_secret = secret;
        ep.authenticated = false;
        ep.last_contact = 0;
        ep.failed_attempts = 0;
        ep.max_failures = 5;
        endpoints[device_id] = ep;
    }

    // Build a sync request message
    MKCommMessage buildSyncRequest(const std::string& target_device,
                                   const std::string& sync_data) const {
        MKCommMessage msg;
        msg.from_device = local_device_id;
        msg.to_device = target_device;
        msg.endpoint = "/api/sync";
        msg.method = "POST";
        msg.body = sync_data;
        msg.sent_at = std::time(nullptr);
        msg.timeout_ms = 30000;

        auto it = endpoints.find(target_device);
        if (it != endpoints.end()) {
            msg.auth_token = generateAuthToken(it->second.shared_secret, msg.sent_at);
        }
        return msg;
    }

    // Build a status request message
    MKCommMessage buildStatusRequest(const std::string& target_device) const {
        MKCommMessage msg;
        msg.from_device = local_device_id;
        msg.to_device = target_device;
        msg.endpoint = "/api/status";
        msg.method = "GET";
        msg.body = "";
        msg.sent_at = std::time(nullptr);
        msg.timeout_ms = 5000;

        auto it = endpoints.find(target_device);
        if (it != endpoints.end()) {
            msg.auth_token = generateAuthToken(it->second.shared_secret, msg.sent_at);
        }
        return msg;
    }

    // Build a task delegation message
    MKCommMessage buildTaskRequest(const std::string& target_device,
                                   const std::string& command,
                                   const std::string& parameters) {
        MKCommMessage msg;
        msg.from_device = local_device_id;
        msg.to_device = target_device;
        msg.endpoint = "/api/task";
        msg.method = "POST";
        msg.body = command + "\n" + parameters;
        msg.sent_at = std::time(nullptr);
        msg.timeout_ms = 60000;

        auto it = endpoints.find(target_device);
        if (it != endpoints.end()) {
            msg.auth_token = generateAuthToken(it->second.shared_secret, msg.sent_at);
        }

        // Track the delegation
        MKTaskDelegation task;
        task.task_id = next_task_id++;
        task.from_device = local_device_id;
        task.to_device = target_device;
        task.command = command;
        task.parameters = parameters;
        task.delegated_at = std::time(nullptr);
        task.completed_at = 0;
        task.completed = false;
        task.failed = false;
        delegated_tasks.push_back(task);

        return msg;
    }

    // Get the full URL for a message
    std::string getFullUrl(const MKCommMessage& msg) const {
        auto it = endpoints.find(msg.to_device);
        if (it == endpoints.end()) return "";
        return it->second.base_url + msg.endpoint;
    }

    // Process a received response
    void processResponse(const std::string& device_id, const MKCommResponse& response) {
        auto it = endpoints.find(device_id);
        if (it != endpoints.end()) {
            if (response.success) {
                it->second.last_contact = std::time(nullptr);
                it->second.failed_attempts = 0;
                it->second.authenticated = true;
            } else {
                it->second.failed_attempts++;
            }
        }
    }

    // Validate an incoming request
    bool validateRequest(const std::string& auth_token) const {
        return verifyAuthToken(auth_token, local_secret);
    }

    // Mark a delegated task as complete
    void completeTask(int task_id, const std::string& result) {
        for (auto& task : delegated_tasks) {
            if (task.task_id == task_id) {
                task.completed = true;
                task.completed_at = std::time(nullptr);
                task.result = result;
                break;
            }
        }
    }

    // Get pending delegated tasks
    std::vector<MKTaskDelegation> getPendingTasks() const {
        std::vector<MKTaskDelegation> pending;
        for (const auto& t : delegated_tasks) {
            if (!t.completed && !t.failed) pending.push_back(t);
        }
        return pending;
    }

    // Get endpoint count
    int endpointCount() const { return static_cast<int>(endpoints.size()); }

    // Get local device ID
    std::string getLocalDeviceId() const { return local_device_id; }

    // Get listen port
    int getListenPort() const { return listen_port; }

    // Check if a device is reachable
    bool isDeviceReachable(const std::string& device_id) const {
        auto it = endpoints.find(device_id);
        if (it == endpoints.end()) return false;
        return it->second.failed_attempts < it->second.max_failures;
    }

    // Get all registered device IDs
    std::vector<std::string> getRegisteredDevices() const {
        std::vector<std::string> devices;
        for (const auto& kv : endpoints) {
            devices.push_back(kv.first);
        }
        return devices;
    }

    // Get summary
    std::string getSummary() const {
        std::ostringstream ss;
        ss << "=== Device Comm ===\n";
        ss << "  Local device: " << local_device_id << " (port " << listen_port << ")\n";
        ss << "  Registered endpoints: " << endpoints.size() << "\n";
        ss << "  Delegated tasks: " << delegated_tasks.size() << "\n";
        int pending = 0;
        for (const auto& t : delegated_tasks) if (!t.completed && !t.failed) pending++;
        ss << "  Pending tasks: " << pending << "\n";
        for (const auto& kv : endpoints) {
            ss << "  " << kv.first << " -> " << kv.second.base_url
               << " (fails: " << kv.second.failed_attempts << ")\n";
        }
        return ss.str();
    }
};

#endif // MK_DEVICE_COMM_CPP
