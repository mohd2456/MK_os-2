#ifndef MK_SERVICE_ORCHESTRATOR_CPP
#define MK_SERVICE_ORCHESTRATOR_CPP

#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <algorithm>
#include <sstream>

// ===================================================================================
// MK SERVICE ORCHESTRATOR
// Manages homelab services (Plex, Home Assistant, Pi-hole, etc.).
// Tracks service health, auto-restarts crashed services, load-balances across nodes,
// handles failover.
// ===================================================================================

enum class MKHomelabServiceStatus {
    RUNNING,
    STOPPED,
    CRASHED,
    RESTARTING,
    DEGRADED,
    UNKNOWN
};

enum class MKServiceType {
    MEDIA,          // Plex, Jellyfin
    HOME_AUTO,      // Home Assistant, OpenHAB
    NETWORK,        // Pi-hole, AdGuard, Unifi
    MONITORING,     // Grafana, Prometheus, Uptime Kuma
    STORAGE,        // NextCloud, MinIO
    SECURITY,       // Authelia, Vaultwarden
    DEVELOPMENT,    // Gitea, Jenkins
    GENERAL
};

struct MKHomelabService {
    std::string name;
    MKServiceType type;
    std::string container_name;
    std::string device_hostname;
    MKHomelabServiceStatus status;
    std::string health_endpoint;     // HTTP endpoint to check health
    int health_port;
    std::time_t last_health_check;
    std::time_t last_restart;
    int restart_count;
    int max_restarts;
    int health_check_interval_sec;
    int consecutive_failures;
    int max_consecutive_failures;
    std::vector<std::string> failover_devices;  // Ordered list of failover targets
    bool auto_restart_enabled;
    float uptime_percent;
    std::time_t started_at;
};

struct MKHealthCheckResult {
    std::string service_name;
    bool healthy;
    int response_time_ms;
    std::string message;
    std::time_t checked_at;
};

struct MKFailoverEvent {
    std::string service_name;
    std::string from_device;
    std::string to_device;
    std::time_t occurred_at;
    std::string reason;
    bool success;
};

class MKServiceOrchestrator {
private:
    std::map<std::string, MKHomelabService> services;
    std::vector<MKHealthCheckResult> health_history;
    std::vector<MKFailoverEvent> failover_history;
    int max_health_history;

    void trimHealthHistory() {
        if ((int)health_history.size() > max_health_history) {
            health_history.erase(health_history.begin(),
                               health_history.begin() + ((int)health_history.size() - max_health_history));
        }
    }

public:
    MKServiceOrchestrator() : max_health_history(1000) {
        // Pre-register common homelab services as templates
        registerService("plex", MKServiceType::MEDIA, "plex", "", 32400, "/web");
        registerService("home_assistant", MKServiceType::HOME_AUTO, "homeassistant", "", 8123, "/api/");
        registerService("pihole", MKServiceType::NETWORK, "pihole", "", 80, "/admin/api.php");
        registerService("grafana", MKServiceType::MONITORING, "grafana", "", 3000, "/api/health");
        registerService("nextcloud", MKServiceType::STORAGE, "nextcloud", "", 443, "/status.php");
        registerService("portainer", MKServiceType::GENERAL, "portainer", "", 9443, "/api/status");
    }

    // Register a homelab service
    void registerService(const std::string& name, MKServiceType type,
                        const std::string& container, const std::string& device,
                        int port = 80, const std::string& health_path = "/") {
        MKHomelabService svc;
        svc.name = name;
        svc.type = type;
        svc.container_name = container;
        svc.device_hostname = device;
        svc.status = MKHomelabServiceStatus::UNKNOWN;
        svc.health_endpoint = health_path;
        svc.health_port = port;
        svc.last_health_check = 0;
        svc.last_restart = 0;
        svc.restart_count = 0;
        svc.max_restarts = 5;
        svc.health_check_interval_sec = 60;
        svc.consecutive_failures = 0;
        svc.max_consecutive_failures = 3;
        svc.auto_restart_enabled = true;
        svc.uptime_percent = 100.0f;
        svc.started_at = std::time(nullptr);
        services[name] = svc;
    }

    // Assign a service to a device
    void assignToDevice(const std::string& service_name, const std::string& device) {
        auto it = services.find(service_name);
        if (it != services.end()) {
            it->second.device_hostname = device;
        }
    }

    // Add failover device for a service
    void addFailoverDevice(const std::string& service_name, const std::string& device) {
        auto it = services.find(service_name);
        if (it != services.end()) {
            it->second.failover_devices.push_back(device);
        }
    }

    // Record a health check result
    void recordHealthCheck(const std::string& service_name, bool healthy,
                          int response_ms = 0, const std::string& msg = "") {
        MKHealthCheckResult result;
        result.service_name = service_name;
        result.healthy = healthy;
        result.response_time_ms = response_ms;
        result.message = msg;
        result.checked_at = std::time(nullptr);
        health_history.push_back(result);
        trimHealthHistory();

        // Update service status
        auto it = services.find(service_name);
        if (it != services.end()) {
            it->second.last_health_check = result.checked_at;
            if (healthy) {
                it->second.status = MKHomelabServiceStatus::RUNNING;
                it->second.consecutive_failures = 0;
            } else {
                it->second.consecutive_failures++;
                if (it->second.consecutive_failures >= it->second.max_consecutive_failures) {
                    it->second.status = MKHomelabServiceStatus::CRASHED;
                } else {
                    it->second.status = MKHomelabServiceStatus::DEGRADED;
                }
            }
        }
    }

    // Check if a service needs restart (returns the docker restart command if so)
    std::string checkNeedsRestart(const std::string& service_name) {
        auto it = services.find(service_name);
        if (it == services.end()) return "";

        MKHomelabService& svc = it->second;
        if (!svc.auto_restart_enabled) return "";
        if (svc.status != MKHomelabServiceStatus::CRASHED) return "";
        if (svc.restart_count >= svc.max_restarts) return "";

        // Generate restart command
        svc.status = MKHomelabServiceStatus::RESTARTING;
        svc.restart_count++;
        svc.last_restart = std::time(nullptr);
        svc.consecutive_failures = 0;

        return "docker restart " + svc.container_name;
    }

    // Check if failover is needed and return target device
    std::string checkNeedsFailover(const std::string& service_name) {
        auto it = services.find(service_name);
        if (it == services.end()) return "";

        MKHomelabService& svc = it->second;
        if (svc.status != MKHomelabServiceStatus::CRASHED) return "";
        if (svc.restart_count < svc.max_restarts) return "";  // Try restarts first
        if (svc.failover_devices.empty()) return "";

        // Pick the first available failover device
        std::string target = svc.failover_devices[0];

        MKFailoverEvent event;
        event.service_name = service_name;
        event.from_device = svc.device_hostname;
        event.to_device = target;
        event.occurred_at = std::time(nullptr);
        event.reason = "Max restarts exceeded on " + svc.device_hostname;
        event.success = false;  // Will be updated when migration completes
        failover_history.push_back(event);

        return target;
    }

    // Mark a failover as successful
    void confirmFailover(const std::string& service_name, const std::string& new_device) {
        auto it = services.find(service_name);
        if (it != services.end()) {
            it->second.device_hostname = new_device;
            it->second.status = MKHomelabServiceStatus::RUNNING;
            it->second.restart_count = 0;
            it->second.consecutive_failures = 0;
        }

        // Update failover history
        for (auto rit = failover_history.rbegin(); rit != failover_history.rend(); ++rit) {
            if (rit->service_name == service_name && !rit->success) {
                rit->success = true;
                break;
            }
        }
    }

    // Get all services
    std::vector<MKHomelabService> getAllServices() const {
        std::vector<MKHomelabService> result;
        for (const auto& kv : services) {
            result.push_back(kv.second);
        }
        return result;
    }

    // Get services by status
    std::vector<MKHomelabService> getServicesByStatus(MKHomelabServiceStatus status) const {
        std::vector<MKHomelabService> result;
        for (const auto& kv : services) {
            if (kv.second.status == status) result.push_back(kv.second);
        }
        return result;
    }

    // Get service count
    int serviceCount() const { return static_cast<int>(services.size()); }

    // Get running service count
    int runningCount() const {
        int count = 0;
        for (const auto& kv : services) {
            if (kv.second.status == MKHomelabServiceStatus::RUNNING) count++;
        }
        return count;
    }

    // Get failover events
    std::vector<MKFailoverEvent> getFailoverHistory() const { return failover_history; }

    // Get summary
    std::string getSummary() const {
        std::ostringstream ss;
        ss << "=== Service Orchestrator ===\n";
        ss << "  Total services: " << services.size() << "\n";
        ss << "  Running: " << runningCount() << "/" << services.size() << "\n";
        ss << "  Failover events: " << failover_history.size() << "\n";
        for (const auto& kv : services) {
            const auto& s = kv.second;
            std::string status_str;
            switch (s.status) {
                case MKHomelabServiceStatus::RUNNING: status_str = "UP"; break;
                case MKHomelabServiceStatus::STOPPED: status_str = "DOWN"; break;
                case MKHomelabServiceStatus::CRASHED: status_str = "CRASH"; break;
                case MKHomelabServiceStatus::DEGRADED: status_str = "DEGRADED"; break;
                case MKHomelabServiceStatus::RESTARTING: status_str = "RESTARTING"; break;
                default: status_str = "?"; break;
            }
            ss << "  " << s.name << " [" << status_str << "] on " << s.device_hostname
               << " (restarts: " << s.restart_count << ")\n";
        }
        return ss.str();
    }
};

#endif // MK_SERVICE_ORCHESTRATOR_CPP
