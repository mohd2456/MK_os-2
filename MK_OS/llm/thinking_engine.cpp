// ============================================================
// MK OS - Thinking Engine (Layer 1)
// ============================================================
// Sends compact reasoning-only prompts to a small model and
// gets back 1-2 sentences of reasoning (NOT a full response).
// The thinking engine decides WHAT to do, not HOW to respond.
//
// Provider preference: local Ollama (phi3:mini) > Groq > OpenRouter
// Fallback: returns empty string (lets decision engine use graph-only reasoning)
// ============================================================
#ifndef MK_THINKING_ENGINE_CPP
#define MK_THINKING_ENGINE_CPP

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <curl/curl.h>

class MKThinkingEngine {
private:
    MKProviderRouter* providerRouter_;
    int maxTokens_;
    float temperature_;
    mutable std::string lastProvider_; // Track which provider was actually used

    // libcurl write callback
    static size_t WriteCallbackThinking(void* contents, size_t size, size_t nmemb, void* userp) {
        if (!userp) return 0;
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    // Extract content from OpenAI-compatible JSON response
    std::string extractContent(const std::string& response) const {
        // Look for "content":"..." in the response
        size_t contentPos = response.find("\"content\":");
        if (contentPos == std::string::npos) return "";

        size_t start = contentPos + 10; // length of "content":
        // Skip whitespace
        while (start < response.size() && (response[start] == ' ' || response[start] == '\t'))
            start++;

        if (start >= response.size()) return "";

        // Check for null
        if (response.substr(start, 4) == "null") return "";

        // Find the opening quote
        if (response[start] != '"') return "";
        start++;

        std::string result;
        for (size_t i = start; i < response.size(); i++) {
            if (response[i] == '\\' && i + 1 < response.size()) {
                // Handle escape sequences
                char next = response[i + 1];
                if (next == '"') { result += '"'; i++; }
                else if (next == 'n') { result += '\n'; i++; }
                else if (next == 't') { result += '\t'; i++; }
                else if (next == '\\') { result += '\\'; i++; }
                else { result += response[i]; }
            } else if (response[i] == '"') {
                break;
            } else {
                result += response[i];
            }
        }
        return result;
    }

    // Pick the best provider for thinking tasks
    // Preference: local Ollama > Groq > OpenRouter > others
    std::string pickThinkingProvider() const {
        if (!providerRouter_) return "";

        // Try local Ollama first (free, fast for thinking)
        if (providerRouter_->isProviderAvailable("ollama")) {
            return "ollama";
        }
        // Groq is fast (good for short reasoning)
        if (providerRouter_->isProviderAvailable("groq")) {
            return "groq";
        }
        // OpenRouter as backup
        if (providerRouter_->isProviderAvailable("openrouter")) {
            return "openrouter";
        }
        // Any available provider via normal routing (LOW urgency, short tokens)
        return providerRouter_->pickProvider(MKRoutingUrgency::LOW, 100);
    }

    // Call the LLM with a thinking prompt
    std::string callProvider(const std::string& providerName, const std::string& prompt) const {
        if (!providerRouter_ || providerName.empty()) return "";

        std::string endpoint = providerRouter_->getProviderEndpoint(providerName);
        std::string model = providerRouter_->getProviderModel(providerName);
        std::string apiKey = providerRouter_->getProviderKey(providerName);

        if (endpoint.empty()) return "";
        if (apiKey.empty() && providerName != "ollama") return "";

        CURL* curl = curl_easy_init();
        if (!curl) return "";

        std::string response;
        std::string body;

        // Escape the prompt for JSON embedding (applies to all providers)
        std::string escapedPrompt;
        for (char c : prompt) {
            if (c == '"') escapedPrompt += "\\\"";
            else if (c == '\n') escapedPrompt += "\\n";
            else if (c == '\r') escapedPrompt += "\\r";
            else if (c == '\t') escapedPrompt += "\\t";
            else if (c == '\\') escapedPrompt += "\\\\";
            else escapedPrompt += c;
        }

        if (providerName == "ollama") {
            // Ollama uses a different format
            body = "{\"model\":\"phi3:mini\","
                   "\"messages\":[{\"role\":\"system\",\"content\":\"You are a reasoning engine. "
                   "Respond in 1-2 sentences only. State what action should be taken.\"},"
                   "{\"role\":\"user\",\"content\":\"" + escapedPrompt + "\"}],"
                   "\"stream\":false,\"options\":{\"num_predict\":" +
                   std::to_string(maxTokens_) + ",\"temperature\":" +
                   std::to_string(temperature_) + "}}";
        } else {
            // OpenAI-compatible format
            body = "{\"model\":\"" + model + "\","
                   "\"messages\":[{\"role\":\"system\",\"content\":\"You are a reasoning engine. "
                   "Respond in 1-2 sentences only. State what action should be taken.\"},"
                   "{\"role\":\"user\",\"content\":\"" + escapedPrompt + "\"}],"
                   "\"max_tokens\":" + std::to_string(maxTokens_) + ","
                   "\"temperature\":" + std::to_string(temperature_) + "}";
        }

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        if (!apiKey.empty()) {
            std::string authHeader = "Authorization: Bearer " + apiKey;
            headers = curl_slist_append(headers, authHeader.c_str());
        }

        curl_easy_setopt(curl, CURLOPT_URL, endpoint.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallbackThinking);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

        CURLcode res = curl_easy_perform(curl);
        long httpCode = 0;
        if (res == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK || httpCode < 200 || httpCode >= 400) {
            return "";
        }

        // Extract the content from the response
        std::string content = extractContent(response);

        // For Ollama, the response format may differ - try "message" -> "content"
        if (content.empty() && providerName == "ollama") {
            size_t msgPos = response.find("\"message\"");
            if (msgPos != std::string::npos) {
                std::string sub = response.substr(msgPos);
                content = extractContent(sub);
            }
        }

        return content;
    }

public:
    MKThinkingEngine()
        : providerRouter_(nullptr), maxTokens_(100), temperature_(0.3f) {}

    void setProviderRouter(MKProviderRouter* router) {
        providerRouter_ = router;
    }

    // Get the provider that was last used for thinking
    std::string getLastProvider() const { return lastProvider_; }

    // Build a compact thinking prompt from user input and context
    std::string buildThinkingPrompt(const std::string& input,
                                     const std::vector<std::string>& facts,
                                     const std::string& systemState) const {
        std::ostringstream prompt;
        prompt << "User asked: " << input << "\n";

        if (!facts.empty()) {
            prompt << "Known facts: ";
            int factCount = 0;
            for (const auto& f : facts) {
                if (factCount >= 5) break; // Limit context size
                prompt << f << "; ";
                factCount++;
            }
            prompt << "\n";
        }

        if (!systemState.empty()) {
            prompt << "System state: " << systemState << "\n";
        }

        prompt << "In 1-2 sentences, what should MK do?";
        return prompt.str();
    }

    // Main thinking method: reason about what to do
    // Returns 1-2 sentence reasoning, or empty string if no LLM available
    std::string think(const std::string& userInput,
                      const std::vector<std::string>& graphContext,
                      const std::string& systemState,
                      const std::string& conversationHistory) const {
        // Build the thinking prompt
        std::string prompt = buildThinkingPrompt(userInput, graphContext, systemState);

        // If we have conversation history, append a brief summary
        if (!conversationHistory.empty()) {
            // Only use last 200 chars of history to keep prompt small
            std::string histSnippet = conversationHistory;
            if (histSnippet.size() > 200) {
                histSnippet = histSnippet.substr(histSnippet.size() - 200);
            }
            prompt = "Context: " + histSnippet + "\n" + prompt;
        }

        // Pick a provider for thinking
        std::string provider = pickThinkingProvider();
        if (provider.empty()) {
            // No LLM available - return empty to trigger fallback
            lastProvider_ = "";
            return "";
        }

        lastProvider_ = provider;

        // Call the provider
        std::string result = callProvider(provider, prompt);

        // Trim and validate - should be short (1-2 sentences)
        if (result.size() > 500) {
            // Truncate overly long responses to first 2 sentences
            std::string periodSpace = ". ";
            size_t firstPeriod = result.find(periodSpace);
            if (firstPeriod != std::string::npos) {
                size_t secondPeriod = result.find(periodSpace, firstPeriod + 2);
                if (secondPeriod != std::string::npos) {
                    result = result.substr(0, secondPeriod + 1);
                } else {
                    result = result.substr(0, firstPeriod + 1);
                }
            }
        }

        return result;
    }

    // Check if the thinking engine has any provider available
    bool isAvailable() const {
        if (!providerRouter_) return false;
        std::string provider = pickThinkingProvider();
        return !provider.empty();
    }

    // Get the max tokens setting
    int getMaxTokens() const { return maxTokens_; }

    // Get the temperature setting
    float getTemperature() const { return temperature_; }
};

#endif // MK_THINKING_ENGINE_CPP
