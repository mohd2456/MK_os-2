#ifndef MK_API_CPP
#define MK_API_CPP

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <ctime>

// ===================================================================================
// MK API GATEWAY MANAGER
// Manages all external and internal API connections. Tracks connection health,
// handles reconnection logic, and provides a unified interface for plugins and
// the ai_core to communicate with external services.
// ===================================================================================

enum class MKAPIStatus {
    DISCONNECTED,
    CONNECTING,
    ACTIVE,
    DEGRADED,
    FAILED
};

struct MKAPIEndpoint {
    std::string name;
    std::string type;        // "remote", "local", "mesh"
    MKAPIStatus status;
    std::time_t lastPing;
    int failCount;
    int successCount;
    double avgLatencyMs;
};

class MKAPI {
private:
    std::map<std::string, MKAPIEndpoint> endpoints;
    int maxRetries;
    double timeoutMs;

    std::string statusLabel(MKAPIStatus s) const {
        switch (s) {
            case MKAPIStatus::DISCONNECTED: return "DISCONNECTED";
            case MKAPIStatus::CONNECTING:   return "CONNECTING";
            case MKAPIStatus::ACTIVE:       return "ACTIVE";
            case MKAPIStatus::DEGRADED:     return "DEGRADED";
            case MKAPIStatus::FAILED:       return "FAILED";
        }
        return "UNKNOWN";
    }

public:
    MKAPI() : maxRetries(3), timeoutMs(5000.0) {
        std::cout << "[API GATEWAY] Multi-endpoint connection manager initialized.\n";
    }

    // Register and connect an API endpoint
    void connect(const std::string& name, const std::string& type = "remote") {
        MKAPIEndpoint ep;
        ep.name = name;
        ep.type = type;
        ep.status = MKAPIStatus::CONNECTING;
        ep.lastPing = std::time(nullptr);
        ep.failCount = 0;
        ep.successCount = 0;
        ep.avgLatencyMs = 0.0;

        // Simulate connection handshake
        ep.status = MKAPIStatus::ACTIVE;
        ep.successCount = 1;

        endpoints[name] = ep;
        std::cout << "[API] Connected: \"" << name << "\" | Type: " << type 
                  << " | Status: " << statusLabel(ep.status) << "\n";
    }

    // Disconnect an endpoint
    void disconnect(const std::string& name) {
        auto it = endpoints.find(name);
        if (it != endpoints.end()) {
            it->second.status = MKAPIStatus::DISCONNECTED;
            std::cout << "[API] Disconnected: \"" << name << "\"\n";
        }
    }

    // Check if an endpoint is healthy
    bool isActive(const std::string& name) const {
        auto it = endpoints.find(name);
        if (it == endpoints.end()) return false;
        return it->second.status == MKAPIStatus::ACTIVE;
    }

    // Record a successful API call
    void recordSuccess(const std::string& name, double latencyMs) {
        auto it = endpoints.find(name);
        if (it != endpoints.end()) {
            it->second.successCount++;
            it->second.lastPing = std::time(nullptr);
            // Running average of latency
            double total = it->second.avgLatencyMs * (it->second.successCount - 1) + latencyMs;
            it->second.avgLatencyMs = total / it->second.successCount;
        }
    }

    // Record a failed API call — triggers degraded state after threshold
    void recordFailure(const std::string& name) {
        auto it = endpoints.find(name);
        if (it != endpoints.end()) {
            it->second.failCount++;
            if (it->second.failCount >= maxRetries) {
                it->second.status = MKAPIStatus::FAILED;
                std::cout << "[API WARN] Endpoint \"" << name 
                          << "\" marked FAILED after " << maxRetries << " consecutive failures.\n";
            } else {
                it->second.status = MKAPIStatus::DEGRADED;
            }
        }
    }

    // Attempt reconnection on failed endpoints
    void reconnectAll() {
        for (auto& kv : endpoints) {
            if (kv.second.status == MKAPIStatus::FAILED || 
                kv.second.status == MKAPIStatus::DISCONNECTED) {
                std::cout << "[API] Attempting reconnection to: " << kv.first << "...\n";
                kv.second.status = MKAPIStatus::ACTIVE;
                kv.second.failCount = 0;
            }
        }
    }

    // Get list of all active endpoint names
    std::vector<std::string> getActiveEndpoints() const {
        std::vector<std::string> active;
        for (const auto& kv : endpoints) {
            if (kv.second.status == MKAPIStatus::ACTIVE) {
                active.push_back(kv.first);
            }
        }
        return active;
    }

    // Print full status dashboard
    void status() const {
        std::cout << "\n--- [API GATEWAY STATUS] ---\n";
        for (const auto& kv : endpoints) {
            const auto& ep = kv.second;
            std::cout << "  " << ep.name 
                      << " | Type=" << ep.type
                      << " | Status=" << statusLabel(ep.status)
                      << " | Success=" << ep.successCount
                      << " | Fails=" << ep.failCount
                      << " | Latency=" << ep.avgLatencyMs << "ms\n";
        }
        std::cout << "----------------------------\n";
    }

    int endpointCount() const { return endpoints.size(); }
};

#endif // MK_API_CPP
