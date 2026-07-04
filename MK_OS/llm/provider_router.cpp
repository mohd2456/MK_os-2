// ============================================================
// MK OS - Multi-Provider API Router
// ============================================================
// Manages multiple LLM providers (Groq, OpenRouter, Nvidia NIM,
// HuggingFace, local Ollama) with intelligent routing based on
// urgency, token estimate, and provider availability.
//
// Features:
// - Urgency-based routing (high = fastest, low = free/local)
// - Token-size routing (short = fast providers, long = local/free)
// - Health tracking with fail counts and latency
// - Daily quota management
// - Usage logging per provider per day
// - Fallback chain when primary provider fails
// ============================================================
#ifndef MK_PROVIDER_ROUTER_CPP
#define MK_PROVIDER_ROUTER_CPP

#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <curl/curl.h>

// Provider routing urgency level
enum class MKRoutingUrgency {
    HIGH,    // Use fastest provider (Groq)
    MEDIUM,  // Balance speed and cost
    LOW      // Use free/local providers
};

// Provider status
enum class MKProviderStatus {
    ONLINE,
    DEGRADED,
    OFFLINE
};

// Provider configuration for the router
struct MKRouterProviderConfig {
    std::string name;
    std::string endpoint;
    std::string model;
    std::string apiKey;
    MKProviderStatus status;
    int dailyQuota;       // Max requests per day (0 = unlimited)
    int usedQuota;        // Requests used today
    float avgLatencyMs;   // Average response time
    int failCount;        // Consecutive failures
    int maxFails;         // Max fails before marking offline
    time_t lastSuccessTime;
    bool isLocal;         // Is this a local provider (Ollama)?
    bool isFree;          // Is this a free-tier provider?
    float speedRating;    // 1.0 = fastest, 0.0 = slowest
};

// Usage log entry
struct MKProviderUsageEntry {
    std::string providerName;
    int tokens;
    float latencyMs;
    bool success;
    time_t timestamp;
};

class MKProviderRouter {
private:
    std::vector<MKRouterProviderConfig> providers_;
    std::vector<MKProviderUsageEntry> usageLog_;
    std::string currentDay_;
    int totalRequests_;
    int successfulRequests_;

    // Get current date as string for daily quota tracking
    std::string getCurrentDay() {
        time_t now = std::time(nullptr);
        struct tm* tm_info = std::localtime(&now);
        char buf[16];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d", tm_info);
        return std::string(buf);
    }

    // Reset daily quotas if date has changed
    void checkDayRollover() {
        std::string today = getCurrentDay();
        if (today != currentDay_) {
            currentDay_ = today;
            for (auto& p : providers_) {
                p.usedQuota = 0;
            }
        }
    }

    // libcurl write callback for test requests
    static size_t WriteCallbackRouter(void* contents, size_t size, size_t nmemb, void* userp) {
        if (!userp) return 0;
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

public:
    MKProviderRouter() : totalRequests_(0), successfulRequests_(0) {
        currentDay_ = getCurrentDay();

        // Configure default providers (order = priority for speed)
        providers_.push_back({
            "groq",                                          // name
            "https://api.groq.com/openai/v1/chat/completions", // endpoint
            // Slugs drift; override with /setmodel groq <slug> instead of recompiling.
            "llama-3.1-8b-instant",                          // model
            "",                                              // apiKey (loaded later)
            MKProviderStatus::OFFLINE,                       // status
            1000,                                            // dailyQuota
            0,                                               // usedQuota
            150.0f,                                          // avgLatencyMs (fast)
            0,                                               // failCount
            5,                                               // maxFails
            0,                                               // lastSuccessTime
            false,                                           // isLocal
            true,                                            // isFree
            0.95f                                            // speedRating (fastest cloud)
        });

        providers_.push_back({
            "openrouter",
            "https://openrouter.ai/api/v1/chat/completions",
            // Slug drift: "meta-llama/llama-3.1-8b-instruct:free" moved to paid (HTTP 404).
            // Default to a valid free small-3B model. Paid alternative (no :free suffix):
            // "meta-llama/llama-3.1-8b-instruct". Override at runtime: /setmodel openrouter <slug>
            "meta-llama/llama-3.2-3b-instruct:free",
            "",
            MKProviderStatus::OFFLINE,
            500,
            0,
            400.0f,
            0,
            5,
            0,
            false,
            true,
            0.7f
        });

        providers_.push_back({
            "nvidia",
            "https://integrate.api.nvidia.com/v1/chat/completions",
            // Slugs drift; override with /setmodel nvidia <slug> instead of recompiling.
            "nvidia/llama-3.1-8b-instruct",
            "",
            MKProviderStatus::OFFLINE,
            1000,
            0,
            300.0f,
            0,
            5,
            0,
            false,
            true,
            0.85f
        });

        providers_.push_back({
            "huggingface",
            "https://api-inference.huggingface.co/models/mistralai/Mistral-7B-Instruct-v0.3/v1/chat/completions",
            "mistralai/Mistral-7B-Instruct-v0.3",
            "",
            MKProviderStatus::OFFLINE,
            300,
            0,
            800.0f,
            0,
            5,
            0,
            false,
            true,
            0.5f
        });

        providers_.push_back({
            "ollama",
            "http://localhost:11434/api/chat",
            "llama3",
            "",
            MKProviderStatus::OFFLINE,
            0,  // unlimited
            0,
            500.0f,
            0,
            3,
            0,
            true,
            true,
            0.6f
        });

        std::cout << "[PROVIDER ROUTER] Initialized with " << providers_.size() << " providers.\n";
    }

    // Set API key for a provider and mark as online
    void setProviderKey(const std::string& providerName, const std::string& key) {
        for (auto& p : providers_) {
            if (p.name == providerName) {
                p.apiKey = key;
                if (!key.empty()) {
                    p.status = MKProviderStatus::ONLINE;
                    p.failCount = 0;
                } else {
                    p.status = MKProviderStatus::OFFLINE;
                }
                return;
            }
        }
    }

    // Set the model slug for a provider at runtime (mirrors setProviderKey).
    // Model slugs drift over time; this lets a stale slug be swapped without a
    // recompile. Returns true if the provider was found.
    bool setProviderModel(const std::string& providerName, const std::string& modelSlug) {
        for (auto& p : providers_) {
            if (p.name == providerName) {
                p.model = modelSlug;
                p.failCount = 0;  // give the new slug a fresh chance
                if (p.status == MKProviderStatus::OFFLINE &&
                    (!p.apiKey.empty() || p.isLocal)) {
                    p.status = MKProviderStatus::DEGRADED;
                }
                return true;
            }
        }
        return false;
    }

    // Get API key for a provider
    std::string getProviderKey(const std::string& providerName) const {
        for (const auto& p : providers_) {
            if (p.name == providerName) {
                return p.apiKey;
            }
        }
        return "";
    }

    // Pick the best provider based on urgency and token estimate
    std::string pickProvider(MKRoutingUrgency urgency, int tokenEstimate) {
        checkDayRollover();

        // Build candidate list: online providers with quota remaining and low fail count
        std::vector<const MKRouterProviderConfig*> candidates;
        for (const auto& p : providers_) {
            if (p.status == MKProviderStatus::OFFLINE) continue;
            if (p.failCount >= p.maxFails) continue;
            if (p.dailyQuota > 0 && p.usedQuota >= p.dailyQuota) continue;
            candidates.push_back(&p);
        }

        if (candidates.empty()) return "";

        // Score each candidate based on urgency and token estimate
        std::string bestProvider;
        float bestScore = -1.0f;

        for (const auto* p : candidates) {
            float score = 0.0f;

            switch (urgency) {
                case MKRoutingUrgency::HIGH:
                    // Prioritize speed
                    score = p->speedRating * 2.0f;
                    // Penalize high latency
                    if (p->avgLatencyMs > 500) score -= 0.5f;
                    break;

                case MKRoutingUrgency::MEDIUM:
                    // Balance speed and availability
                    score = p->speedRating;
                    // Bonus for low fail count
                    score += (1.0f - (float)p->failCount / (float)p->maxFails) * 0.5f;
                    break;

                case MKRoutingUrgency::LOW:
                    // Prioritize free/local
                    if (p->isLocal) score += 2.0f;
                    if (p->isFree) score += 1.0f;
                    // Penalize fast expensive providers
                    if (!p->isFree && !p->isLocal) score -= 0.5f;
                    break;
            }

            // Token size adjustments
            if (tokenEstimate < 100) {
                // Short requests: prefer fast providers
                score += p->speedRating * 0.5f;
            } else if (tokenEstimate > 500) {
                // Long requests: prefer local or free (no quota burn)
                if (p->isLocal) score += 1.0f;
                if (p->dailyQuota == 0) score += 0.5f;
            }

            // Availability bonus: lower fail count = more reliable
            score += (1.0f - (float)p->failCount / (float)std::max(p->maxFails, 1)) * 0.3f;

            // Quota remaining bonus
            if (p->dailyQuota > 0) {
                float quotaRemaining = 1.0f - (float)p->usedQuota / (float)p->dailyQuota;
                score += quotaRemaining * 0.2f;
            }

            if (score > bestScore) {
                bestScore = score;
                bestProvider = p->name;
            }
        }

        return bestProvider;
    }

    // Test a provider with a minimal API call
    bool testProvider(const std::string& providerName) {
        MKRouterProviderConfig* provider = nullptr;
        for (auto& p : providers_) {
            if (p.name == providerName) {
                provider = &p;
                break;
            }
        }
        if (!provider) return false;
        if (provider->apiKey.empty() && !provider->isLocal) return false;

        // Build a minimal test request
        CURL* curl = curl_easy_init();
        if (!curl) return false;

        std::string response;
        std::string url = provider->endpoint;
        std::string body;

        if (provider->isLocal) {
            // Ollama format
            body = "{\"model\":\"" + provider->model + "\",\"messages\":[{\"role\":\"user\",\"content\":\"hi\"}],\"stream\":false}";
        } else {
            // OpenAI-compatible format
            body = "{\"model\":\"" + provider->model + "\","
                   "\"messages\":[{\"role\":\"user\",\"content\":\"hi\"}],"
                   "\"max_tokens\":5,\"temperature\":0.1}";
        }

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        if (!provider->apiKey.empty()) {
            std::string authHeader = "Authorization: Bearer " + provider->apiKey;
            headers = curl_slist_append(headers, authHeader.c_str());
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallbackRouter);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

        CURLcode res = curl_easy_perform(curl);
        long httpCode = 0;
        if (res == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        bool success = (res == CURLE_OK && httpCode >= 200 && httpCode < 400);

        if (success) {
            provider->status = MKProviderStatus::ONLINE;
            provider->failCount = 0;
            provider->lastSuccessTime = std::time(nullptr);
        } else {
            provider->failCount++;
            if (provider->failCount >= provider->maxFails) {
                provider->status = MKProviderStatus::OFFLINE;
            } else {
                provider->status = MKProviderStatus::DEGRADED;
            }
        }

        return success;
    }

    // Get a formatted status report for all providers
    std::string getStatusReport() {
        checkDayRollover();

        std::ostringstream report;
        report << "<b>Provider Status</b>\n\n";

        for (const auto& p : providers_) {
            // Status icon
            std::string icon;
            switch (p.status) {
                case MKProviderStatus::ONLINE:   icon = "✅"; break;
                case MKProviderStatus::DEGRADED: icon = "⚠️"; break;
                case MKProviderStatus::OFFLINE:  icon = "❌"; break;
            }

            report << icon << " <b>" << p.name << "</b>";
            if (p.isLocal) report << " (local)";
            report << "\n";
            report << "   Model: <code>" << p.model << "</code>\n";

            // Status details
            if (p.status == MKProviderStatus::ONLINE) {
                report << "   Status: ONLINE";
            } else if (p.status == MKProviderStatus::DEGRADED) {
                report << "   Status: DEGRADED (fails: " << p.failCount << ")";
            } else {
                if (p.apiKey.empty() && !p.isLocal) {
                    report << "   Status: NO KEY";
                } else {
                    report << "   Status: OFFLINE";
                }
            }
            report << "\n";

            // Quota
            if (p.dailyQuota > 0) {
                report << "   Quota: " << p.usedQuota << "/" << p.dailyQuota << " today\n";
            } else {
                report << "   Quota: unlimited\n";
            }

            report << "   Latency: ~" << (int)p.avgLatencyMs << "ms\n";
            report << "\n";
        }

        report << "<b>Routing:</b> " << successfulRequests_ << "/" << totalRequests_ << " successful\n";

        return report.str();
    }

    // Log a request outcome
    void logRequest(const std::string& providerName, int tokens, float latencyMs, bool success) {
        checkDayRollover();

        // Update provider stats
        for (auto& p : providers_) {
            if (p.name == providerName) {
                if (success) {
                    p.failCount = 0;
                    p.lastSuccessTime = std::time(nullptr);
                    p.usedQuota++;
                    // Running average latency
                    p.avgLatencyMs = p.avgLatencyMs * 0.8f + latencyMs * 0.2f;
                    p.status = MKProviderStatus::ONLINE;
                } else {
                    p.failCount++;
                    if (p.failCount >= p.maxFails) {
                        p.status = MKProviderStatus::OFFLINE;
                    } else {
                        p.status = MKProviderStatus::DEGRADED;
                    }
                }
                break;
            }
        }

        // Add to usage log
        usageLog_.push_back({providerName, tokens, latencyMs, success, std::time(nullptr)});

        // Keep log manageable
        if (usageLog_.size() > 1000) {
            usageLog_.erase(usageLog_.begin(), usageLog_.begin() + 500);
        }

        totalRequests_++;
        if (success) successfulRequests_++;
    }

    // Get fallback chain: ordered list of providers to try if primary fails
    std::vector<std::string> getFallbackChain(const std::string& failedProvider) {
        checkDayRollover();
        std::vector<std::string> chain;

        // Sort available providers by speed rating (descending)
        std::vector<std::pair<float, std::string>> scored;
        for (const auto& p : providers_) {
            if (p.name == failedProvider) continue;
            if (p.status == MKProviderStatus::OFFLINE) continue;
            if (p.failCount >= p.maxFails) continue;
            if (p.dailyQuota > 0 && p.usedQuota >= p.dailyQuota) continue;
            scored.push_back({p.speedRating, p.name});
        }

        std::sort(scored.begin(), scored.end(),
                  [](const auto& a, const auto& b) { return a.first > b.first; });

        for (const auto& s : scored) {
            chain.push_back(s.second);
        }

        return chain;
    }

    // Get provider endpoint URL
    std::string getProviderEndpoint(const std::string& providerName) const {
        for (const auto& p : providers_) {
            if (p.name == providerName) return p.endpoint;
        }
        return "";
    }

    // Get provider model
    std::string getProviderModel(const std::string& providerName) const {
        for (const auto& p : providers_) {
            if (p.name == providerName) return p.model;
        }
        return "";
    }

    // Check if a provider is available (online with quota)
    bool isProviderAvailable(const std::string& providerName) const {
        for (const auto& p : providers_) {
            if (p.name == providerName) {
                if (p.status == MKProviderStatus::OFFLINE) return false;
                if (p.failCount >= p.maxFails) return false;
                if (p.dailyQuota > 0 && p.usedQuota >= p.dailyQuota) return false;
                return true;
            }
        }
        return false;
    }

    // Check if a provider is healthy (online, low fail count, not recently erroring)
    bool isProviderHealthy(const std::string& providerName) const {
        for (const auto& p : providers_) {
            if (p.name == providerName) {
                if (p.status == MKProviderStatus::OFFLINE) return false;
                if (p.failCount >= p.maxFails) return false;
                if (p.dailyQuota > 0 && p.usedQuota >= p.dailyQuota) return false;
                // Additional health check: more than 2 consecutive failures means unhealthy
                if (p.failCount > 2) return false;
                return true;
            }
        }
        return false;
    }

    // Check if a provider name is valid
    bool isValidProvider(const std::string& name) const {
        for (const auto& p : providers_) {
            if (p.name == name) return true;
        }
        return false;
    }

    // Get list of all provider names
    std::vector<std::string> getProviderNames() const {
        std::vector<std::string> names;
        for (const auto& p : providers_) {
            names.push_back(p.name);
        }
        return names;
    }

    // Get the active (current best) provider name
    std::string getActiveProvider() const {
        for (const auto& p : providers_) {
            if (p.status == MKProviderStatus::ONLINE && p.failCount < p.maxFails) {
                return p.name;
            }
        }
        return "none";
    }

    // Get total request count
    int getTotalRequests() const { return totalRequests_; }
    int getSuccessfulRequests() const { return successfulRequests_; }

    // Get provider count
    int providerCount() const { return (int)providers_.size(); }

    // Get online provider count
    int onlineCount() const {
        int count = 0;
        for (const auto& p : providers_) {
            if (p.status == MKProviderStatus::ONLINE) count++;
        }
        return count;
    }

    // Reset fail counts for all providers (periodic recovery)
    void resetFailCounts() {
        for (auto& p : providers_) {
            if (p.failCount > 0 && p.status != MKProviderStatus::OFFLINE) {
                p.failCount = 0;
            }
            // Re-enable offline providers that have keys
            if (p.status == MKProviderStatus::OFFLINE && (!p.apiKey.empty() || p.isLocal)) {
                p.status = MKProviderStatus::DEGRADED;
                p.failCount = 0;
            }
        }
    }
};

#endif // MK_PROVIDER_ROUTER_CPP
