#ifndef MK_WEB_MONITOR_CPP
#define MK_WEB_MONITOR_CPP

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <algorithm>
#include <functional>
#include <sstream>
#include <cstdint>

// ===================================================================================
// MK WEB MONITOR PLUGIN
// Watches websites/URLs and alerts when content changes.
// Features:
//   - User adds URLs to monitor with configurable check intervals
//   - Periodically fetches URLs and compares content hashes
//   - Detects changes via simple hash comparison
//   - Alerts via console output (Telegram integration optional)
//   - Use cases: job postings, product prices, news, API status pages
//   - Stores full history of detected changes with timestamps
// ===================================================================================

enum class MonitorStatus {
    ACTIVE,
    PAUSED,
    ERROR,
    REMOVED
};

enum class ChangeType {
    CONTENT_CHANGED,
    CONTENT_ADDED,
    CONTENT_REMOVED,
    STATUS_CHANGED,
    FIRST_CHECK
};

struct ContentChange {
    std::time_t detectedAt;
    ChangeType type;
    uint64_t previousHash;
    uint64_t newHash;
    int previousSize;
    int newSize;
    std::string summary;
};

struct MonitoredURL {
    std::string url;
    std::string label;              // User-friendly name
    int checkIntervalSeconds;       // How often to check
    MonitorStatus status;
    std::time_t addedAt;
    std::time_t lastCheckedAt;
    std::time_t lastChangedAt;
    uint64_t lastContentHash;
    int lastContentSize;
    int totalChecks;
    int totalChangesDetected;
    std::vector<ContentChange> changeHistory;
    std::string lastError;
};

struct MonitorAlert {
    std::string url;
    std::string label;
    ChangeType type;
    std::time_t alertTime;
    std::string message;
    bool acknowledged;
};

class MKWebMonitor {
private:
    std::vector<MonitoredURL> monitors;
    std::vector<MonitorAlert> alerts;
    int totalChecksPerformed;
    int totalChangesDetected;
    int totalErrors;
    bool telegramEnabled;
    std::string telegramBotToken;
    std::string telegramChatId;

    // Simple hash function for content comparison (FNV-1a inspired)
    uint64_t hashContent(const std::string& content) const {
        uint64_t hash = 14695981039346656037ULL;
        for (char c : content) {
            hash ^= static_cast<uint64_t>(c);
            hash *= 1099511628211ULL;
        }
        return hash;
    }

    // Simulate fetching URL content (in production would use MKHTTP)
    std::string fetchURL(const std::string& url, bool& success) {
        // This would integrate with MKHTTP in the full system
        // For now, we document the interface
        std::cout << "[WEB_MONITOR] Fetching: " << url << "\n";
        success = true;
        return ""; // Placeholder - real implementation uses MKHTTP::get()
    }

    // Generate alert message
    std::string generateAlertMessage(const MonitoredURL& monitor,
                                      const ContentChange& change) const {
        std::string msg;
        msg += "[WEB_MONITOR ALERT] Change detected!\n";
        msg += "  URL: " + monitor.url + "\n";
        msg += "  Label: " + monitor.label + "\n";
        msg += "  Type: ";
        switch (change.type) {
            case ChangeType::CONTENT_CHANGED: msg += "Content Modified"; break;
            case ChangeType::CONTENT_ADDED: msg += "New Content Added"; break;
            case ChangeType::CONTENT_REMOVED: msg += "Content Removed"; break;
            case ChangeType::STATUS_CHANGED: msg += "Status Changed"; break;
            case ChangeType::FIRST_CHECK: msg += "Initial Baseline"; break;
        }
        msg += "\n";
        msg += "  Size change: " + std::to_string(change.previousSize) +
               " -> " + std::to_string(change.newSize) + " bytes\n";
        msg += "  Detected at: " + std::string(std::ctime(&change.detectedAt));
        return msg;
    }

public:
    MKWebMonitor() : totalChecksPerformed(0), totalChangesDetected(0),
                      totalErrors(0), telegramEnabled(false) {
        std::cout << "[WEB_MONITOR] Initialized - website change detection system\n";
        std::cout << "[WEB_MONITOR] Add URLs to begin monitoring\n";
    }

    // Add a URL to monitor
    bool addURL(const std::string& url, const std::string& label,
                int intervalSeconds = 300) {
        // Check for duplicates
        for (const auto& m : monitors) {
            if (m.url == url && m.status == MonitorStatus::ACTIVE) {
                std::cout << "[WEB_MONITOR] URL already being monitored: " << url << "\n";
                return false;
            }
        }

        MonitoredURL monitor;
        monitor.url = url;
        monitor.label = label;
        monitor.checkIntervalSeconds = intervalSeconds;
        monitor.status = MonitorStatus::ACTIVE;
        monitor.addedAt = std::time(nullptr);
        monitor.lastCheckedAt = 0;
        monitor.lastChangedAt = 0;
        monitor.lastContentHash = 0;
        monitor.lastContentSize = 0;
        monitor.totalChecks = 0;
        monitor.totalChangesDetected = 0;

        monitors.push_back(monitor);
        std::cout << "[WEB_MONITOR] Added: " << label << " (" << url << ")\n";
        std::cout << "[WEB_MONITOR] Check interval: " << intervalSeconds << "s\n";
        return true;
    }

    // Remove a URL from monitoring
    bool removeURL(const std::string& url) {
        for (auto& m : monitors) {
            if (m.url == url && m.status == MonitorStatus::ACTIVE) {
                m.status = MonitorStatus::REMOVED;
                std::cout << "[WEB_MONITOR] Removed: " << m.label << " (" << url << ")\n";
                return true;
            }
        }
        std::cout << "[WEB_MONITOR] URL not found: " << url << "\n";
        return false;
    }

    // Pause monitoring for a URL
    bool pauseURL(const std::string& url) {
        for (auto& m : monitors) {
            if (m.url == url && m.status == MonitorStatus::ACTIVE) {
                m.status = MonitorStatus::PAUSED;
                std::cout << "[WEB_MONITOR] Paused: " << m.label << "\n";
                return true;
            }
        }
        return false;
    }

    // Resume monitoring for a URL
    bool resumeURL(const std::string& url) {
        for (auto& m : monitors) {
            if (m.url == url && m.status == MonitorStatus::PAUSED) {
                m.status = MonitorStatus::ACTIVE;
                std::cout << "[WEB_MONITOR] Resumed: " << m.label << "\n";
                return true;
            }
        }
        return false;
    }

    // Check a specific URL for changes
    bool checkURL(MonitoredURL& monitor) {
        if (monitor.status != MonitorStatus::ACTIVE) {
            return false;
        }

        bool fetchSuccess = false;
        std::string content = fetchURL(monitor.url, fetchSuccess);

        if (!fetchSuccess) {
            monitor.lastError = "Failed to fetch URL";
            monitor.status = MonitorStatus::ERROR;
            totalErrors++;
            std::cout << "[WEB_MONITOR] ERROR fetching: " << monitor.url << "\n";
            return false;
        }

        monitor.lastCheckedAt = std::time(nullptr);
        monitor.totalChecks++;
        totalChecksPerformed++;

        uint64_t newHash = hashContent(content);
        int newSize = static_cast<int>(content.size());

        // First check - establish baseline
        if (monitor.lastContentHash == 0) {
            ContentChange change;
            change.detectedAt = std::time(nullptr);
            change.type = ChangeType::FIRST_CHECK;
            change.previousHash = 0;
            change.newHash = newHash;
            change.previousSize = 0;
            change.newSize = newSize;
            change.summary = "Initial baseline established";
            monitor.changeHistory.push_back(change);
            monitor.lastContentHash = newHash;
            monitor.lastContentSize = newSize;
            std::cout << "[WEB_MONITOR] Baseline set for: " << monitor.label << "\n";
            return false;
        }

        // Compare with previous hash
        if (newHash != monitor.lastContentHash) {
            ContentChange change;
            change.detectedAt = std::time(nullptr);
            change.previousHash = monitor.lastContentHash;
            change.newHash = newHash;
            change.previousSize = monitor.lastContentSize;
            change.newSize = newSize;

            // Determine change type
            if (newSize > monitor.lastContentSize + 100) {
                change.type = ChangeType::CONTENT_ADDED;
                change.summary = "Content grew by " +
                    std::to_string(newSize - monitor.lastContentSize) + " bytes";
            } else if (newSize < monitor.lastContentSize - 100) {
                change.type = ChangeType::CONTENT_REMOVED;
                change.summary = "Content shrunk by " +
                    std::to_string(monitor.lastContentSize - newSize) + " bytes";
            } else {
                change.type = ChangeType::CONTENT_CHANGED;
                change.summary = "Content modified (similar size)";
            }

            monitor.changeHistory.push_back(change);
            monitor.lastContentHash = newHash;
            monitor.lastContentSize = newSize;
            monitor.lastChangedAt = std::time(nullptr);
            monitor.totalChangesDetected++;
            totalChangesDetected++;

            // Create alert
            MonitorAlert alert;
            alert.url = monitor.url;
            alert.label = monitor.label;
            alert.type = change.type;
            alert.alertTime = std::time(nullptr);
            alert.message = generateAlertMessage(monitor, change);
            alert.acknowledged = false;
            alerts.push_back(alert);

            std::cout << alert.message;
            return true;
        }

        return false; // No change
    }

    // Run a check cycle on all active monitors
    int checkAll() {
        std::cout << "[WEB_MONITOR] Running check cycle on "
                  << monitors.size() << " monitors\n";
        int changesFound = 0;
        std::time_t now = std::time(nullptr);

        for (auto& monitor : monitors) {
            if (monitor.status != MonitorStatus::ACTIVE) continue;

            // Check if enough time has passed since last check
            double elapsed = std::difftime(now, monitor.lastCheckedAt);
            if (elapsed >= monitor.checkIntervalSeconds) {
                if (checkURL(monitor)) {
                    changesFound++;
                }
            }
        }

        std::cout << "[WEB_MONITOR] Check cycle complete. Changes found: "
                  << changesFound << "\n";
        return changesFound;
    }

    // Get unacknowledged alerts
    std::vector<MonitorAlert> getUnacknowledgedAlerts() const {
        std::vector<MonitorAlert> unacked;
        for (const auto& a : alerts) {
            if (!a.acknowledged) unacked.push_back(a);
        }
        return unacked;
    }

    // Acknowledge all alerts
    void acknowledgeAlerts() {
        for (auto& a : alerts) {
            a.acknowledged = true;
        }
        std::cout << "[WEB_MONITOR] All alerts acknowledged\n";
    }

    // Configure Telegram alerts
    void configureTelegram(const std::string& botToken, const std::string& chatId) {
        telegramBotToken = botToken;
        telegramChatId = chatId;
        telegramEnabled = true;
        std::cout << "[WEB_MONITOR] Telegram alerts configured\n";
    }

    // Get change history for a URL
    std::vector<ContentChange> getHistory(const std::string& url) const {
        for (const auto& m : monitors) {
            if (m.url == url) {
                return m.changeHistory;
            }
        }
        return {};
    }

    // Update check interval for a URL
    bool setInterval(const std::string& url, int seconds) {
        for (auto& m : monitors) {
            if (m.url == url) {
                m.checkIntervalSeconds = seconds;
                std::cout << "[WEB_MONITOR] Updated interval for " << m.label
                          << ": " << seconds << "s\n";
                return true;
            }
        }
        return false;
    }

    // List all monitored URLs
    void listMonitors() const {
        std::cout << "\n=== MONITORED URLS ===\n";
        for (const auto& m : monitors) {
            std::string status;
            switch (m.status) {
                case MonitorStatus::ACTIVE: status = "ACTIVE"; break;
                case MonitorStatus::PAUSED: status = "PAUSED"; break;
                case MonitorStatus::ERROR: status = "ERROR"; break;
                case MonitorStatus::REMOVED: status = "REMOVED"; break;
            }
            std::cout << "  [" << status << "] " << m.label << "\n";
            std::cout << "    URL: " << m.url << "\n";
            std::cout << "    Interval: " << m.checkIntervalSeconds << "s\n";
            std::cout << "    Checks: " << m.totalChecks
                      << " | Changes: " << m.totalChangesDetected << "\n";
        }
        std::cout << "======================\n\n";
    }

    // Print stats
    void printStats() const {
        std::cout << "\n=== WEB MONITOR STATS ===\n";
        std::cout << "Active monitors: ";
        int active = 0;
        for (const auto& m : monitors) {
            if (m.status == MonitorStatus::ACTIVE) active++;
        }
        std::cout << active << "\n";
        std::cout << "Total checks performed: " << totalChecksPerformed << "\n";
        std::cout << "Total changes detected: " << totalChangesDetected << "\n";
        std::cout << "Total errors: " << totalErrors << "\n";
        std::cout << "Unacknowledged alerts: " << getUnacknowledgedAlerts().size() << "\n";
        std::cout << "Telegram alerts: " << (telegramEnabled ? "ON" : "OFF") << "\n";
        std::cout << "=========================\n\n";
    }
};

#endif // MK_WEB_MONITOR_CPP
