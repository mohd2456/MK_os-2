// ============================================================
// MK OS - LLM Engine
// Manages connection to local LLM servers (llama.cpp or Ollama).
// Auto-detects available backends and provides text generation.
// ============================================================
#ifndef MK_LLM_ENGINE_CPP
#define MK_LLM_ENGINE_CPP

#include <iostream>
#include <string>
#include <curl/curl.h>
#include "model_manager.cpp"

// ANSI color codes (defined inline to avoid dependency on Color:: namespace
// which is declared in mk_entry.cpp and may not be available at include time)
namespace LLMColor {
    const char* RESET   = "\033[0m";
    const char* BOLD    = "\033[1m";
    const char* DIM     = "\033[2m";
    const char* GREEN   = "\033[32m";
    const char* YELLOW  = "\033[33m";
    const char* CYAN    = "\033[36m";
    const char* BGREEN  = "\033[92m";
    const char* BYELLOW = "\033[93m";
    const char* BCYAN   = "\033[96m";
}

class MKLLMEngine {
private:
    MKModelManager modelManager;
    std::string serverType; // "ollama", "llama.cpp", or "none"
    bool available;
    std::string ollamaModel; // configurable model name for Ollama

    // --------------------------------------------------------
    // JSON-escape a string for embedding in JSON payloads
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
    // Parse a JSON string value from a response given the key
    // --------------------------------------------------------
    std::string parseJsonStringValue(const std::string& json, const std::string& key) {
        std::string searchKey = "\"" + key + "\":\"";
        size_t startPos = json.find(searchKey);
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
                    default:   result += next; break;
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
    // libcurl write callback
    // --------------------------------------------------------
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        if (!userp) return 0;
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    // --------------------------------------------------------
    // Check if a URL responds with HTTP 200
    // --------------------------------------------------------
    bool checkEndpoint(const std::string& url) {
        CURL* curl = curl_easy_init();
        if (!curl) return false;

        std::string response;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 1L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

        CURLcode res = curl_easy_perform(curl);
        long httpCode = 0;
        if (res == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        }
        curl_easy_cleanup(curl);

        return (res == CURLE_OK && httpCode == 200);
    }

    // --------------------------------------------------------
    // Detect which LLM server is available
    // --------------------------------------------------------
    void detectServer() {
        // Check llama.cpp server first (localhost:8080)
        if (checkEndpoint("http://localhost:8080/health")) {
            serverType = "llama.cpp";
            available = true;
            return;
        }

        // Check Ollama (localhost:11434)
        if (checkEndpoint("http://localhost:11434/api/tags")) {
            serverType = "ollama";
            available = true;
            return;
        }

        serverType = "none";
        available = false;
    }

public:
    MKLLMEngine() : serverType("none"), available(false), ollamaModel("tinyllama") {
        std::cout << "[LLM ENGINE] Initializing local LLM integration...\n";
        detectServer();
    }

    // --------------------------------------------------------
    // Check if an LLM server is available
    // --------------------------------------------------------
    bool isAvailable() const {
        return available;
    }

    // --------------------------------------------------------
    // Get the type of server connected
    // --------------------------------------------------------
    std::string getServerType() const {
        return serverType;
    }

    // --------------------------------------------------------
    // Get reference to the model manager
    // --------------------------------------------------------
    const MKModelManager& getModelManager() const {
        return modelManager;
    }

    // --------------------------------------------------------
    // Set/get Ollama model name
    // --------------------------------------------------------
    void setOllamaModel(const std::string& model) {
        ollamaModel = model;
    }

    std::string getOllamaModel() const {
        return ollamaModel;
    }

    // --------------------------------------------------------
    // Generate text using the available LLM server
    // --------------------------------------------------------
    std::string generate(const std::string& prompt) {
        if (!available) {
            return "";
        }

        CURL* curl = curl_easy_init();
        if (!curl) return "";

        std::string response;
        std::string url;
        std::string postData;
        std::string escapedPrompt = jsonEscape(prompt);

        if (serverType == "ollama") {
            // Use /api/chat with system/user role separation
            url = "http://localhost:11434/api/chat";
            postData = "{\"model\":\"" + jsonEscape(ollamaModel) + "\","
                "\"messages\":["
                "{\"role\":\"user\",\"content\":\"" + escapedPrompt + "\"}"
                "],\"stream\":false}";
        } else if (serverType == "llama.cpp") {
            url = "http://localhost:8080/completion";
            postData = "{\"prompt\":\"" + escapedPrompt +
                "\",\"n_predict\":256,\"temperature\":0.7}";
        } else {
            return "";
        }

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

        CURLcode res = curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            return "";
        }

        // Parse response to extract generated text
        // Ollama /api/chat format: {"message":{"role":"assistant","content":"..."}}
        // llama.cpp format: {"content":"..."}
        return parseJsonStringValue(response, "content");
    }

    // --------------------------------------------------------
    // Print boot-time status check with colored output
    // --------------------------------------------------------
    void checkAndReport() {
        // Check hardware first
        if (!modelManager.checkHardware()) {
            std::cout << "  " << LLMColor::BYELLOW << "!" << LLMColor::RESET
                      << " LLM: Hardware insufficient (" << modelManager.getRAMString()
                      << " free, need " << modelManager.getMinRAM() << " GB minimum).\n"
                      << "  " << LLMColor::DIM
                      << "  Local AI features disabled to prevent system instability."
                      << LLMColor::RESET << "\n";
            return;
        }

        if (!available) {
            std::cout << "  " << LLMColor::DIM << "Note:" << LLMColor::RESET
                      << " No local LLM server detected. "
                      << LLMColor::DIM << "Run " << LLMColor::CYAN
                      << "llm/start_server.sh" << LLMColor::RESET
                      << LLMColor::DIM << " to enable AI features."
                      << LLMColor::RESET << "\n";
        } else {
            std::cout << "  " << LLMColor::BGREEN << "+" << LLMColor::RESET
                      << " LLM: Connected to " << LLMColor::BOLD << serverType
                      << LLMColor::RESET << " server. "
                      << LLMColor::DIM << "(" << modelManager.getRAMString() << " available)"
                      << LLMColor::RESET << "\n";
        }
    }

    // --------------------------------------------------------
    // Re-detect server (useful after user starts a server)
    // --------------------------------------------------------
    void refresh() {
        detectServer();
    }
};

#endif // MK_LLM_ENGINE_CPP
