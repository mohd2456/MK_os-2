#ifndef MK_SERVICE_MANAGER_CPP
#define MK_SERVICE_MANAGER_CPP

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <deque>
#include <memory>
#include <ctime>
#include <algorithm>
#include <sstream>
#include <functional>
#include <queue>

namespace MK_Services {

// ─── Service Status ─────────────────────────────────────────────────────────

enum class ServiceStatus {
    STOPPED,
    STARTING,
    RUNNING,
    STOPPING,
    CRASHED,
    RESTARTING,
    DISABLED
};

// ─── Health Check ───────────────────────────────────────────────────────────

enum class HealthState {
    HEALTHY,
    DEGRADED,
    UNHEALTHY,
    UNKNOWN
};

struct HealthCheck {
    uint32_t interval_seconds = 30;
    uint32_t timeout_seconds = 5;
    uint32_t retries = 3;
    uint32_t consecutive_failures = 0;
    time_t last_check = 0;
    HealthState state = HealthState::UNKNOWN;
};


// ─── Log Entry ──────────────────────────────────────────────────────────────

struct ServiceLog {
    time_t timestamp;
    std::string service_name;
    std::string message;
    std::string level; // INFO, WARN, ERROR
};

// ─── Service Definition ─────────────────────────────────────────────────────

struct Service {
    std::string name;
    std::string description;
    std::string start_command;
    std::string stop_command;
    std::vector<std::string> dependencies;
    ServiceStatus status = ServiceStatus::STOPPED;
    HealthCheck health;
    uint32_t pid = 0;
    time_t started_at = 0;
    time_t stopped_at = 0;
    uint32_t restart_count = 0;
    uint32_t max_restarts = 5;
    uint32_t restart_backoff_ms = 1000;
    uint32_t current_backoff_ms = 1000;
    bool auto_restart = true;
    bool enabled = true;
    int priority = 0; // higher = starts first
    std::deque<ServiceLog> logs;
    size_t max_log_entries = 1000;

    void add_log(const std::string& message, const std::string& level = "INFO") {
        ServiceLog entry;
        entry.timestamp = std::time(nullptr);
        entry.service_name = name;
        entry.message = message;
        entry.level = level;
        logs.push_back(entry);
        if (logs.size() > max_log_entries) logs.pop_front();
    }
};

// ─── Service Manager ────────────────────────────────────────────────────────

class ServiceManager {
private:
    std::map<std::string, std::shared_ptr<Service>> services_;
    std::vector<std::string> startup_order_;
    uint32_t next_pid_ = 1000;
    bool shutting_down_ = false;

    // Topological sort for dependency-ordered startup
    bool compute_startup_order() {
        startup_order_.clear();
        std::map<std::string, int> in_degree;
        std::map<std::string, std::vector<std::string>> dependents;

        for (const auto& p : services_) {
            if (!p.second->enabled) continue;
            in_degree[p.first] = 0;
        }
        for (const auto& p : services_) {
            if (!p.second->enabled) continue;
            for (const auto& dep : p.second->dependencies) {
                if (in_degree.count(dep)) {
                    in_degree[p.first]++;
                    dependents[dep].push_back(p.first);
                }
            }
        }

        std::queue<std::string> ready;
        for (const auto& p : in_degree) {
            if (p.second == 0) ready.push(p.first);
        }

        while (!ready.empty()) {
            std::string current = ready.front();
            ready.pop();
            startup_order_.push_back(current);
            for (const auto& dep : dependents[current]) {
                if (--in_degree[dep] == 0) {
                    ready.push(dep);
                }
            }
        }

        return startup_order_.size() == in_degree.size(); // false = circular dep
    }

    bool dependencies_running(const std::shared_ptr<Service>& svc) const {
        for (const auto& dep : svc->dependencies) {
            auto it = services_.find(dep);
            if (it == services_.end()) return false;
            if (it->second->status != ServiceStatus::RUNNING) return false;
        }
        return true;
    }

public:
    ServiceManager() = default;

    // ── Registration ──

    bool register_service(const std::string& name, const std::string& start_cmd,
                          const std::vector<std::string>& deps = {},
                          const std::string& description = "") {
        if (services_.count(name)) return false;
        auto svc = std::make_shared<Service>();
        svc->name = name;
        svc->start_command = start_cmd;
        svc->dependencies = deps;
        svc->description = description;
        services_[name] = svc;
        svc->add_log("Service registered");
        return true;
    }

    bool unregister_service(const std::string& name) {
        auto it = services_.find(name);
        if (it == services_.end()) return false;
        if (it->second->status == ServiceStatus::RUNNING) {
            stop_service(name);
        }
        services_.erase(it);
        return true;
    }


    // ── Start/Stop ──

    bool start_service(const std::string& name) {
        auto it = services_.find(name);
        if (it == services_.end()) return false;
        auto& svc = it->second;
        if (svc->status == ServiceStatus::RUNNING) return true;
        if (!svc->enabled) return false;

        if (!dependencies_running(svc)) {
            svc->add_log("Cannot start: dependencies not met", "ERROR");
            return false;
        }

        svc->status = ServiceStatus::STARTING;
        svc->add_log("Starting service...");
        // Simulate process start
        svc->pid = next_pid_++;
        svc->started_at = std::time(nullptr);
        svc->status = ServiceStatus::RUNNING;
        svc->health.state = HealthState::HEALTHY;
        svc->health.consecutive_failures = 0;
        svc->add_log("Service started (PID " + std::to_string(svc->pid) + ")");
        return true;
    }

    bool stop_service(const std::string& name) {
        auto it = services_.find(name);
        if (it == services_.end()) return false;
        auto& svc = it->second;
        if (svc->status != ServiceStatus::RUNNING && svc->status != ServiceStatus::STARTING) return false;

        svc->status = ServiceStatus::STOPPING;
        svc->add_log("Stopping service...");
        // Simulate graceful stop
        svc->stopped_at = std::time(nullptr);
        svc->pid = 0;
        svc->status = ServiceStatus::STOPPED;
        svc->add_log("Service stopped");
        return true;
    }

    bool restart_service(const std::string& name) {
        stop_service(name);
        return start_service(name);
    }

    // ── Auto-Restart ──

    void handle_crash(const std::string& name) {
        auto it = services_.find(name);
        if (it == services_.end()) return;
        auto& svc = it->second;

        svc->status = ServiceStatus::CRASHED;
        svc->pid = 0;
        svc->stopped_at = std::time(nullptr);
        svc->add_log("Service crashed!", "ERROR");

        if (svc->auto_restart && svc->restart_count < svc->max_restarts) {
            svc->restart_count++;
            svc->current_backoff_ms = svc->restart_backoff_ms * svc->restart_count;
            svc->status = ServiceStatus::RESTARTING;
            svc->add_log("Auto-restarting (attempt " + std::to_string(svc->restart_count) +
                        "/" + std::to_string(svc->max_restarts) + ")", "WARN");
            start_service(name);
        } else {
            svc->add_log("Max restarts exceeded, staying crashed", "ERROR");
        }
    }

    // ── Health Checks ──

    void run_health_checks() {
        for (auto& p : services_) {
            auto& svc = p.second;
            if (svc->status != ServiceStatus::RUNNING) continue;

            time_t now = std::time(nullptr);
            if (now - svc->health.last_check < svc->health.interval_seconds) continue;
            svc->health.last_check = now;

            // Simulate health check (always passes in simulation)
            bool healthy = true; // Would actually ping the service
            if (healthy) {
                svc->health.consecutive_failures = 0;
                svc->health.state = HealthState::HEALTHY;
            } else {
                svc->health.consecutive_failures++;
                if (svc->health.consecutive_failures >= svc->health.retries) {
                    svc->health.state = HealthState::UNHEALTHY;
                    svc->add_log("Health check failed, marking unhealthy", "ERROR");
                    handle_crash(p.first);
                } else {
                    svc->health.state = HealthState::DEGRADED;
                    svc->add_log("Health check degraded", "WARN");
                }
            }
        }
    }

    // ── Startup/Shutdown Sequences ──

    bool start_all() {
        if (!compute_startup_order()) return false;
        for (const auto& name : startup_order_) {
            start_service(name);
        }
        return true;
    }

    void shutdown_all() {
        shutting_down_ = true;
        // Reverse order shutdown
        std::vector<std::string> order = startup_order_;
        std::reverse(order.begin(), order.end());
        for (const auto& name : order) {
            stop_service(name);
        }
        shutting_down_ = false;
    }

    // ── Status & Info ──

    ServiceStatus get_status(const std::string& name) const {
        auto it = services_.find(name);
        if (it == services_.end()) return ServiceStatus::STOPPED;
        return it->second->status;
    }

    std::shared_ptr<Service> get_service(const std::string& name) {
        auto it = services_.find(name);
        return (it != services_.end()) ? it->second : nullptr;
    }

    std::vector<std::string> list_services() const {
        std::vector<std::string> names;
        for (const auto& p : services_) names.push_back(p.first);
        return names;
    }

    std::vector<std::pair<std::string, ServiceStatus>> get_all_status() const {
        std::vector<std::pair<std::string, ServiceStatus>> result;
        for (const auto& p : services_) {
            result.push_back({p.first, p.second->status});
        }
        return result;
    }

    std::deque<ServiceLog> get_logs(const std::string& name, size_t count = 50) const {
        auto it = services_.find(name);
        if (it == services_.end()) return {};
        const auto& logs = it->second->logs;
        if (logs.size() <= count) return logs;
        return std::deque<ServiceLog>(logs.end() - count, logs.end());
    }

    // ── Enable/Disable ──

    bool enable_service(const std::string& name) {
        auto it = services_.find(name);
        if (it == services_.end()) return false;
        it->second->enabled = true;
        return true;
    }

    bool disable_service(const std::string& name) {
        auto it = services_.find(name);
        if (it == services_.end()) return false;
        it->second->enabled = false;
        if (it->second->status == ServiceStatus::RUNNING) {
            stop_service(name);
        }
        it->second->status = ServiceStatus::DISABLED;
        return true;
    }

    size_t running_count() const {
        size_t count = 0;
        for (const auto& p : services_) {
            if (p.second->status == ServiceStatus::RUNNING) count++;
        }
        return count;
    }

    size_t total_count() const { return services_.size(); }
};

} // namespace MK_Services

#endif // MK_SERVICE_MANAGER_CPP
