#ifndef MK_FRESHNESS_MANAGER_CPP
#define MK_FRESHNESS_MANAGER_CPP

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <ctime>
#include <algorithm>

// ===================================================================================
// MK FRESHNESS MANAGER - FACT STALENESS TRACKER
// Tracks when each fact was last verified and assigns expiry periods by category.
// Ensures MK's knowledge stays current by flagging stale facts for re-verification.
// Category expiry periods:
//   - Science: 365 days (changes slowly)
//   - Technology: 90 days (evolves quickly)
//   - Politics: 7 days (volatile)
//   - Sports: 1 day (results change daily)
//   - General: 180 days (moderate refresh)
// ===================================================================================

enum class MKFactCategory {
    SCIENCE,
    TECHNOLOGY,
    POLITICS,
    SPORTS,
    GENERAL
};

struct MKFreshnessRecord {
    std::string factKey;
    MKFactCategory category;
    std::time_t lastVerified;
    std::time_t expiresAt;
    bool flaggedForReverification;
    int verificationCount;
};

class MKFreshnessManager {
private:
    std::unordered_map<std::string, MKFreshnessRecord> records;
    int totalTracked;
    int totalStale;
    int totalReverifications;

    // Expiry periods in seconds for each category
    long getExpirySeconds(MKFactCategory category) {
        switch (category) {
            case MKFactCategory::SCIENCE:    return 365L * 24 * 3600;
            case MKFactCategory::TECHNOLOGY: return 90L * 24 * 3600;
            case MKFactCategory::POLITICS:   return 7L * 24 * 3600;
            case MKFactCategory::SPORTS:     return 1L * 24 * 3600;
            case MKFactCategory::GENERAL:    return 180L * 24 * 3600;
        }
        return 180L * 24 * 3600; // default to general
    }

    std::string categoryToString(MKFactCategory cat) {
        switch (cat) {
            case MKFactCategory::SCIENCE:    return "science";
            case MKFactCategory::TECHNOLOGY: return "technology";
            case MKFactCategory::POLITICS:   return "politics";
            case MKFactCategory::SPORTS:     return "sports";
            case MKFactCategory::GENERAL:    return "general";
        }
        return "general";
    }

    // Detect category from topic keywords
    MKFactCategory detectCategory(const std::string& topic) {
        std::string lower = topic;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        // Science keywords
        if (lower.find("physics") != std::string::npos ||
            lower.find("chemistry") != std::string::npos ||
            lower.find("biology") != std::string::npos ||
            lower.find("math") != std::string::npos ||
            lower.find("scientific") != std::string::npos ||
            lower.find("quantum") != std::string::npos ||
            lower.find("evolution") != std::string::npos ||
            lower.find("molecule") != std::string::npos ||
            lower.find("atom") != std::string::npos) {
            return MKFactCategory::SCIENCE;
        }

        // Technology keywords
        if (lower.find("software") != std::string::npos ||
            lower.find("programming") != std::string::npos ||
            lower.find("computer") != std::string::npos ||
            lower.find("ai") != std::string::npos ||
            lower.find("machine learning") != std::string::npos ||
            lower.find("internet") != std::string::npos ||
            lower.find("app") != std::string::npos ||
            lower.find("api") != std::string::npos ||
            lower.find("cloud") != std::string::npos ||
            lower.find("code") != std::string::npos) {
            return MKFactCategory::TECHNOLOGY;
        }

        // Politics keywords
        if (lower.find("president") != std::string::npos ||
            lower.find("election") != std::string::npos ||
            lower.find("government") != std::string::npos ||
            lower.find("senate") != std::string::npos ||
            lower.find("congress") != std::string::npos ||
            lower.find("political") != std::string::npos ||
            lower.find("law") != std::string::npos ||
            lower.find("policy") != std::string::npos) {
            return MKFactCategory::POLITICS;
        }

        // Sports keywords
        if (lower.find("game") != std::string::npos ||
            lower.find("score") != std::string::npos ||
            lower.find("player") != std::string::npos ||
            lower.find("team") != std::string::npos ||
            lower.find("championship") != std::string::npos ||
            lower.find("match") != std::string::npos ||
            lower.find("tournament") != std::string::npos ||
            lower.find("league") != std::string::npos) {
            return MKFactCategory::SPORTS;
        }

        return MKFactCategory::GENERAL;
    }

public:
    MKFreshnessManager() : totalTracked(0), totalStale(0), totalReverifications(0) {
        std::cout << "[FRESHNESS] Fact freshness tracking manager initialized.\n";
    }

    // Register a fact with its verification timestamp
    void registerFact(const std::string& factKey, const std::string& topic) {
        MKFreshnessRecord record;
        record.factKey = factKey;
        record.category = detectCategory(topic);
        record.lastVerified = std::time(nullptr);
        record.expiresAt = record.lastVerified + getExpirySeconds(record.category);
        record.flaggedForReverification = false;
        record.verificationCount = 1;

        records[factKey] = record;
        totalTracked++;
    }

    // Check if a fact is stale (past its expiry)
    bool isStale(const std::string& factKey) {
        auto it = records.find(factKey);
        if (it == records.end()) return true; // Unknown facts are considered stale

        std::time_t now = std::time(nullptr);
        bool stale = (now > it->second.expiresAt);
        if (stale && !it->second.flaggedForReverification) {
            it->second.flaggedForReverification = true;
            totalStale++;
        }
        return stale;
    }

    // Get the expiry category for a topic
    MKFactCategory getExpiryCategory(const std::string& topic) {
        return detectCategory(topic);
    }

    // Get expiry period in days for a category
    int getExpiryDays(MKFactCategory category) {
        return (int)(getExpirySeconds(category) / (24 * 3600));
    }

    // Flag a specific fact for re-verification
    void flagForReverification(const std::string& factKey) {
        auto it = records.find(factKey);
        if (it != records.end()) {
            it->second.flaggedForReverification = true;
            totalReverifications++;
            std::cout << "[FRESHNESS] Flagged for reverification: " << factKey << "\n";
        }
    }

    // Mark a fact as freshly verified (resets expiry)
    void markVerified(const std::string& factKey) {
        auto it = records.find(factKey);
        if (it != records.end()) {
            it->second.lastVerified = std::time(nullptr);
            it->second.expiresAt = it->second.lastVerified + getExpirySeconds(it->second.category);
            it->second.flaggedForReverification = false;
            it->second.verificationCount++;
        }
    }

    // Get list of all stale facts needing re-verification
    std::vector<std::string> getStaleFactsList() {
        std::vector<std::string> staleKeys;
        std::time_t now = std::time(nullptr);
        for (auto& pair : records) {
            if (now > pair.second.expiresAt) {
                staleKeys.push_back(pair.first);
                pair.second.flaggedForReverification = true;
            }
        }
        return staleKeys;
    }

    // Get all facts flagged for reverification
    std::vector<std::string> getFlaggedFacts() {
        std::vector<std::string> flagged;
        for (const auto& pair : records) {
            if (pair.second.flaggedForReverification) {
                flagged.push_back(pair.first);
            }
        }
        return flagged;
    }

    // Get time until a fact expires (negative means already expired)
    long getTimeUntilExpiry(const std::string& factKey) {
        auto it = records.find(factKey);
        if (it == records.end()) return -1;
        std::time_t now = std::time(nullptr);
        return (long)std::difftime(it->second.expiresAt, now);
    }

    void printStats() const {
        std::cout << "[FRESHNESS STATS] Tracked: " << totalTracked
                  << " | Stale: " << totalStale
                  << " | Reverifications: " << totalReverifications << "\n";
    }
};

#endif // MK_FRESHNESS_MANAGER_CPP
