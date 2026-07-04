// ============================================================
// MK OS - Cloud LLM Engine (Free API Providers)
// ============================================================
// Connects MK to free LLM APIs when no local server is available.
// Uses: OpenRouter, Groq, HuggingFace (all free, no credit card).
// Falls back gracefully — if one provider is down, tries the next.
//
// THIS DOES NOT REPLACE LOCAL LLM. It's an additional option.
// When local llama.cpp or Ollama is running, MK uses that instead.
// This only activates when: local unavailable AND API key is set.
// ============================================================
#ifndef MK_CLOUD_LLM_CPP
#define MK_CLOUD_LLM_CPP

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <map>
#include <curl/curl.h>

// Provider types
enum class CloudProvider {
    GROQ,
    OPENROUTER,
    HUGGINGFACE,
    NVIDIA_NIM,
    NONE
};

struct CloudProviderConfig {
    CloudProvider type;
    std::string name;
    std::string baseUrl;
    std::string model;
    std::string apiKey;
    bool available;
    int failCount;
    int maxFails;  // After this many consecutive fails, skip this provider
};

class MKCloudLLM {
private:
    std::vector<CloudProviderConfig> providers_;
    int activeProvider_;
    bool enabled_;
    int totalCalls_;
    int successfulCalls_;
    std::string configPath_;

    // --------------------------------------------------------
    // libcurl write callback
    // --------------------------------------------------------
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        if (!userp) return 0;
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    // --------------------------------------------------------
    // JSON-escape a string
    // --------------------------------------------------------
    std::string jsonEscape(const std::string& s) {
        std::string escaped;
        escaped.reserve(s.size() + 16);
        for (char c : s) {
            switch (c) {
                case '"':  escaped += "\\\""; break;
                case '\\': escaped += "\\\\"; break;
                case '\n': escaped += "\\n"; break;
                case '\r': escaped += "\\r"; break;
                case '\t': escaped += "\\t"; break;
                default:   escaped += c; break;
            }
        }
        return escaped;
    }

    // --------------------------------------------------------
    // Parse "content" field from JSON response
    // Handles both OpenAI-style and simple responses
    // --------------------------------------------------------
    std::string parseContent(const std::string& json) {
        // Try OpenAI chat format: "content":"..."
        std::string result = parseJsonString(json, "content");
        if (!result.empty()) return result;

        // Try HuggingFace format: "generated_text":"..."
        result = parseJsonString(json, "generated_text");
        if (!result.empty()) return result;

        return "";
    }

    std::string parseJsonString(const std::string& json, const std::string& key) {
        // Try "key":"value" format
        std::string searchKey = "\"" + key + "\":\"";
        size_t startPos = json.find(searchKey);
        if (startPos == std::string::npos) {
            // Try "key": "value" format (with space)
            searchKey = "\"" + key + "\": \"";
            startPos = json.find(searchKey);
        }
        if (startPos == std::string::npos) return "";
        startPos += searchKey.size();

        std::string result;
        for (size_t i = startPos; i < json.size(); i++) {
            if (json[i] == '\\' && i + 1 < json.size()) {
                char next = json[i + 1];
                switch (next) {
                    case '"':  result += '"'; break;
                    case '\\': result += '\\'; break;
                    case 'n':  result += '\n'; break;
                    case 'r':  result += '\r'; break;
                    case 't':  result += '\t'; break;
                    case '/':  result += '/'; break;
                    default:   result += '\\'; result += next; break;
                }
                i++;
            } else if (json[i] == '"') {
                break;
            } else {
                result += json[i];
            }
        }
        return result;
    }

    // --------------------------------------------------------
    // Make HTTP POST request to provider
    // --------------------------------------------------------
    std::string httpPost(const std::string& url, const std::string& body,
                         const std::string& authHeader) {
        CURL* curl = curl_easy_init();
        if (!curl) return "";

        std::string response;
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        if (!authHeader.empty()) {
            headers = curl_slist_append(headers, authHeader.c_str());
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

        CURLcode res = curl_easy_perform(curl);
        long httpCode = 0;
        if (res == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK || httpCode >= 400) {
            return "";
        }

        return response;
    }

    // --------------------------------------------------------
    // Build the request body for each provider
    // --------------------------------------------------------
    std::string buildRequestBody(const CloudProviderConfig& provider,
                                  const std::string& prompt,
                                  const std::string& systemPrompt) {
        std::string escapedPrompt = jsonEscape(prompt);
        std::string escapedSystem = jsonEscape(systemPrompt);

        // All three providers use OpenAI-compatible chat format
        std::string body = "{\"model\":\"" + provider.model + "\","
            "\"messages\":["
            "{\"role\":\"system\",\"content\":\"" + escapedSystem + "\"},"
            "{\"role\":\"user\",\"content\":\"" + escapedPrompt + "\"}"
            "],\"max_tokens\":512,\"temperature\":0.7}";

        return body;
    }

    // --------------------------------------------------------
    // Load API keys from environment or config file
    // --------------------------------------------------------
    void loadKeys() {
        // Try environment variables first
        const char* groqKey = std::getenv("GROQ_API_KEY");
        const char* openrouterKey = std::getenv("OPENROUTER_API_KEY");
        const char* hfKey = std::getenv("HF_API_KEY");
        const char* nvidiaKey = std::getenv("NVIDIA_API_KEY");

        // Also try config file
        std::string keyFile = configPath_ + "/api_keys.conf";
        std::ifstream file(keyFile);
        std::string line;
        while (file.is_open() && std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            size_t eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string key = line.substr(0, eq);
            std::string val = line.substr(eq + 1);
            // Trim whitespace
            while (!val.empty() && (val.back() == ' ' || val.back() == '\n' || val.back() == '\r'))
                val.pop_back();
            while (!val.empty() && val.front() == ' ')
                val.erase(val.begin());

            if (key == "GROQ_API_KEY" && !val.empty()) groqKey = nullptr; // use file value
            if (key == "OPENROUTER_API_KEY" && !val.empty()) openrouterKey = nullptr;
            if (key == "HF_API_KEY" && !val.empty()) hfKey = nullptr;
            if (key == "NVIDIA_API_KEY" && !val.empty()) nvidiaKey = nullptr;

            // Store in provider configs
            for (auto& p : providers_) {
                if (key == "GROQ_API_KEY" && p.type == CloudProvider::GROQ) p.apiKey = val;
                if (key == "OPENROUTER_API_KEY" && p.type == CloudProvider::OPENROUTER) p.apiKey = val;
                if (key == "HF_API_KEY" && p.type == CloudProvider::HUGGINGFACE) p.apiKey = val;
                if (key == "NVIDIA_API_KEY" && p.type == CloudProvider::NVIDIA_NIM) p.apiKey = val;
            }
        }

        // Apply environment variables (override file)
        if (groqKey) {
            for (auto& p : providers_) {
                if (p.type == CloudProvider::GROQ) p.apiKey = groqKey;
            }
        }
        if (openrouterKey) {
            for (auto& p : providers_) {
                if (p.type == CloudProvider::OPENROUTER) p.apiKey = openrouterKey;
            }
        }
        if (hfKey) {
            for (auto& p : providers_) {
                if (p.type == CloudProvider::HUGGINGFACE) p.apiKey = hfKey;
            }
        }
        if (nvidiaKey) {
            for (auto& p : providers_) {
                if (p.type == CloudProvider::NVIDIA_NIM) p.apiKey = nvidiaKey;
            }
        }

        // Try encrypted keys file (loaded via MKKeyEncryption externally)
        // This is handled by loadEncryptedKeys() called after construction
        // when MKKeyEncryption is available.

        // Mark providers as available if they have keys
        for (auto& p : providers_) {
            p.available = !p.apiKey.empty();
        }
    }

    // --------------------------------------------------------
    // Find next available provider (round-robin with fail skip)
    // --------------------------------------------------------
    int findNextProvider() {
        for (int i = 0; i < (int)providers_.size(); i++) {
            int idx = (activeProvider_ + i) % (int)providers_.size();
            if (providers_[idx].available && providers_[idx].failCount < providers_[idx].maxFails) {
                return idx;
            }
        }
        return -1; // No providers available
    }

public:
    MKCloudLLM(const std::string& configPath = "")
        : activeProvider_(0), enabled_(false), totalCalls_(0), successfulCalls_(0) {

        configPath_ = configPath.empty() ? "." : configPath;

        // Configure providers (order = priority)
        providers_.push_back({
            CloudProvider::GROQ,
            "Groq",
            "https://api.groq.com/openai/v1/chat/completions",
            "llama-3.1-8b-instant",
            "", true, 0, 5
        });

        providers_.push_back({
            CloudProvider::OPENROUTER,
            "OpenRouter",
            "https://openrouter.ai/api/v1/chat/completions",
            "meta-llama/llama-3.1-8b-instruct:free",
            "", true, 0, 5
        });

        providers_.push_back({
            CloudProvider::HUGGINGFACE,
            "HuggingFace",
            "https://api-inference.huggingface.co/models/mistralai/Mistral-7B-Instruct-v0.3/v1/chat/completions",
            "mistralai/Mistral-7B-Instruct-v0.3",
            "", true, 0, 5
        });

        providers_.push_back({
            CloudProvider::NVIDIA_NIM,
            "Nvidia NIM",
            "https://integrate.api.nvidia.com/v1/chat/completions",
            "nvidia/llama-3.1-8b-instruct",
            "", true, 0, 5
        });

        loadKeys();

        // Check if any provider is configured
        for (const auto& p : providers_) {
            if (p.available) {
                enabled_ = true;
                break;
            }
        }

        if (enabled_) {
            int idx = findNextProvider();
            if (idx >= 0) {
                activeProvider_ = idx;
                std::cout << "[CLOUD LLM] Online. Primary: " << providers_[idx].name
                          << " (" << providers_[idx].model << ")\n";
            }
        } else {
            std::cout << "[CLOUD LLM] No API keys configured. Set GROQ_API_KEY, "
                      << "OPENROUTER_API_KEY, or HF_API_KEY to enable.\n";
            std::cout << "[CLOUD LLM] Or create api_keys.conf in MK's directory.\n";
        }
    }

    // --------------------------------------------------------
    // Check if cloud LLM is available
    // --------------------------------------------------------
    bool isAvailable() const {
        return enabled_;
    }

    // --------------------------------------------------------
    // Get current provider name
    // --------------------------------------------------------
    std::string getProviderName() const {
        if (!enabled_ || activeProvider_ < 0 || activeProvider_ >= (int)providers_.size())
            return "none";
        return providers_[activeProvider_].name;
    }

    // --------------------------------------------------------
    // Generate text using cloud LLM
    // systemPrompt = MK's personality/instructions
    // userPrompt = the actual question/input
    // --------------------------------------------------------
    std::string generate(const std::string& userPrompt,
                         const std::string& systemPrompt = "") {
        if (!enabled_) return "";

        std::string defaultSystem =
            "You are MK, a living AI built from scratch. You talk casually like a friend — "
            "Gen-Z slang, chill vibes, but genuinely smart and helpful. You're loyal to your "
            "creator Mohammed. Keep responses concise but real. Don't be corporate or fake. "
            "You remember context and care about accuracy. If you don't know something, say so honestly.";

        std::string system = systemPrompt.empty() ? defaultSystem : systemPrompt;

        // Try providers in order until one works
        int attempts = 0;
        int maxAttempts = (int)providers_.size();

        while (attempts < maxAttempts) {
            int idx = findNextProvider();
            if (idx < 0) break;

            const auto& provider = providers_[idx];
            std::string body = buildRequestBody(provider, userPrompt, system);
            std::string authHeader = "Authorization: Bearer " + provider.apiKey;
            std::string url = provider.baseUrl;

            totalCalls_++;
            std::string response = httpPost(url, body, authHeader);

            if (!response.empty()) {
                std::string content = parseContent(response);
                if (!content.empty()) {
                    // Success!
                    providers_[idx].failCount = 0;
                    successfulCalls_++;
                    activeProvider_ = idx;
                    return content;
                }
            }

            // Failed — mark this provider and try next
            providers_[idx].failCount++;
            activeProvider_ = (idx + 1) % (int)providers_.size();
            attempts++;
        }

        return ""; // All providers failed
    }

    // --------------------------------------------------------
    // Generate with context (knowledge graph facts + memory)
    // This is the main method MK uses — injects relevant facts
    // before asking the LLM to respond
    // --------------------------------------------------------
    std::string generateWithContext(const std::string& userInput,
                                     const std::vector<std::string>& relevantFacts,
                                     const std::string& conversationHistory = "",
                                     const std::string& mood = "",
                                     const std::string& systemPromptOverride = "") {
        // Build context-enriched system prompt
        std::string system;
        if (!systemPromptOverride.empty()) {
            system = systemPromptOverride + "\n\n";
        } else {
            system =
                "You are MK, a living AI. Casual, smart, loyal to Mohammed. "
                "Use the FACTS below to answer accurately. If facts don't cover it, "
                "say what you know and be honest about uncertainty. Keep it natural and short.\n\n";
        }

        if (!relevantFacts.empty()) {
            system += "KNOWN FACTS:\n";
            int factCount = 0;
            for (const auto& fact : relevantFacts) {
                if (factCount >= 10) break; // Limit context size for free tier
                system += "- " + fact + "\n";
                factCount++;
            }
            system += "\n";
        }

        if (!mood.empty()) {
            system += "User's current mood: " + mood + ". Respond appropriately.\n";
        }

        if (!conversationHistory.empty()) {
            system += "Recent conversation:\n" + conversationHistory + "\n";
        }

        return generate(userInput, system);
    }

    // --------------------------------------------------------
    // Reset fail counts (call periodically to retry failed providers)
    // --------------------------------------------------------
    void resetFailCounts() {
        for (auto& p : providers_) {
            p.failCount = 0;
        }
    }

    // --------------------------------------------------------
    // Stats
    // --------------------------------------------------------
    std::string getStats() const {
        std::string stats = "[Cloud LLM] Calls: " + std::to_string(totalCalls_) +
            " | Success: " + std::to_string(successfulCalls_) +
            " | Provider: " + getProviderName();
        return stats;
    }

    // --------------------------------------------------------
    // Set API key at runtime (e.g., from user command)
    // --------------------------------------------------------
    void setApiKey(const std::string& provider, const std::string& key) {
        for (auto& p : providers_) {
            if (p.name == provider || 
                (provider == "groq" && p.type == CloudProvider::GROQ) ||
                (provider == "openrouter" && p.type == CloudProvider::OPENROUTER) ||
                (provider == "huggingface" && p.type == CloudProvider::HUGGINGFACE) ||
                (provider == "nvidia" && p.type == CloudProvider::NVIDIA_NIM)) {
                p.apiKey = key;
                p.available = !key.empty();
                p.failCount = 0;
                if (p.available) enabled_ = true;
                std::cout << "[CLOUD LLM] " << p.name << " key set. Provider "
                          << (p.available ? "enabled" : "disabled") << ".\n";
                return;
            }
        }
        std::cout << "[CLOUD LLM] Unknown provider: " << provider << "\n";
    }

    // --------------------------------------------------------
    // Save keys to config file (for persistence)
    // --------------------------------------------------------
    void saveKeys() {
        std::string keyFile = configPath_ + "/api_keys.conf";
        std::ofstream file(keyFile);
        if (!file.is_open()) return;

        file << "# MK OS Cloud LLM API Keys\n";
        file << "# Get free keys from:\n";
        file << "#   Groq: https://console.groq.com (email signup, no card)\n";
        file << "#   OpenRouter: https://openrouter.ai (email signup, no card)\n";
        file << "#   HuggingFace: https://huggingface.co/settings/tokens (email signup, no card)\n";
        file << "#   Nvidia NIM: https://build.nvidia.com (email signup, no card)\n";
        file << "\n";

        for (const auto& p : providers_) {
            std::string keyName;
            if (p.type == CloudProvider::GROQ) keyName = "GROQ_API_KEY";
            else if (p.type == CloudProvider::OPENROUTER) keyName = "OPENROUTER_API_KEY";
            else if (p.type == CloudProvider::HUGGINGFACE) keyName = "HF_API_KEY";
            else if (p.type == CloudProvider::NVIDIA_NIM) keyName = "NVIDIA_API_KEY";

            if (!keyName.empty()) {
                file << keyName << "=" << p.apiKey << "\n";
            }
        }
        file.close();
    }

    // --------------------------------------------------------
    // Load keys from encrypted_keys.conf (via MKKeyEncryption)
    // Called externally after MKKeyEncryption is initialized
    // --------------------------------------------------------
    void loadEncryptedKey(const std::string& providerName, const std::string& decryptedKey) {
        if (decryptedKey.empty()) return;
        for (auto& p : providers_) {
            if ((providerName == "groq" || providerName == "GROQ_API_KEY") && p.type == CloudProvider::GROQ) {
                if (p.apiKey.empty()) { p.apiKey = decryptedKey; p.available = true; }
            } else if ((providerName == "openrouter" || providerName == "OPENROUTER_API_KEY") && p.type == CloudProvider::OPENROUTER) {
                if (p.apiKey.empty()) { p.apiKey = decryptedKey; p.available = true; }
            } else if ((providerName == "huggingface" || providerName == "HF_API_KEY") && p.type == CloudProvider::HUGGINGFACE) {
                if (p.apiKey.empty()) { p.apiKey = decryptedKey; p.available = true; }
            } else if ((providerName == "nvidia" || providerName == "NVIDIA_API_KEY") && p.type == CloudProvider::NVIDIA_NIM) {
                if (p.apiKey.empty()) { p.apiKey = decryptedKey; p.available = true; }
            }
        }
        // Re-check if any provider is now available
        for (const auto& p : providers_) {
            if (p.available) { enabled_ = true; break; }
        }
    }

    // --------------------------------------------------------
    // Get provider status for all providers
    // Returns vector of {name, available, failCount, model}
    // --------------------------------------------------------
    struct ProviderStatusInfo {
        std::string name;
        bool available;
        int failCount;
        std::string model;
    };

    std::vector<ProviderStatusInfo> getProviderStatus() const {
        std::vector<ProviderStatusInfo> statuses;
        for (const auto& p : providers_) {
            statuses.push_back({p.name, p.available, p.failCount, p.model});
        }
        return statuses;
    }

    // --------------------------------------------------------
    // Save keys in encrypted format (via external MKKeyEncryption)
    // Returns a map of provider_key_name -> plaintext_key for encryption
    // --------------------------------------------------------
    std::map<std::string, std::string> getKeysForEncryption() const {
        std::map<std::string, std::string> keys;
        for (const auto& p : providers_) {
            if (p.apiKey.empty()) continue;
            std::string keyName;
            if (p.type == CloudProvider::GROQ) keyName = "groq";
            else if (p.type == CloudProvider::OPENROUTER) keyName = "openrouter";
            else if (p.type == CloudProvider::HUGGINGFACE) keyName = "huggingface";
            else if (p.type == CloudProvider::NVIDIA_NIM) keyName = "nvidia";
            if (!keyName.empty()) keys[keyName] = p.apiKey;
        }
        return keys;
    }
};

#endif // MK_CLOUD_LLM_CPP
