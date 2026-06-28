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
    MKLLMEngine() : serverType("none"), available(false) {
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

        if (serverType == "ollama") {
            url = "http://localhost:11434/api/generate";
            // Build JSON payload for Ollama
            postData = "{\"model\":\"tinyllama\",\"prompt\":\"";
            // Escape the prompt for JSON
            for (char c : prompt) {
                switch (c) {
                    case '"':  postData += "\\\""; break;
                    case '\\': postData += "\\\\"; break;
                    case '\n': postData += "\\n"; break;
                    case '\r': postData += "\\r"; break;
                    case '\t': postData += "\\t"; break;
                    default:   postData += c; break;
                }
            }
            postData += "\",\"stream\":false}";
        } else if (serverType == "llama.cpp") {
            url = "http://localhost:8080/completion";
            // Build JSON payload for llama.cpp
            postData = "{\"prompt\":\"";
            for (char c : prompt) {
                switch (c) {
                    case '"':  postData += "\\\""; break;
                    case '\\': postData += "\\\\"; break;
                    case '\n': postData += "\\n"; break;
                    case '\r': postData += "\\r"; break;
                    case '\t': postData += "\\t"; break;
                    default:   postData += c; break;
                }
            }
            postData += "\",\"n_predict\":256,\"temperature\":0.7}";
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
        // Ollama format: {"response":"..."}
        // llama.cpp format: {"content":"..."}
        std::string searchKey;
        if (serverType == "ollama") {
            searchKey = "\"response\":\"";
        } else {
            searchKey = "\"content\":\"";
        }

        size_t startPos = response.find(searchKey);
        if (startPos == std::string::npos) return response;
        startPos += searchKey.size();

        // Find the end of the value (handle escaped quotes)
        std::string result;
        for (size_t i = startPos; i < response.size(); i++) {
            if (response[i] == '\\' && i + 1 < response.size()) {
                char next = response[i + 1];
                switch (next) {
                    case '"':  result += '"'; break;
                    case '\\': result += '\\'; break;
                    case 'n':  result += '\n'; break;
                    case 'r':  result += '\r'; break;
                    case 't':  result += '\t'; break;
                    default:   result += next; break;
                }
                i++; // skip the escaped character
            } else if (response[i] == '"') {
                break; // end of string value
            } else {
                result += response[i];
            }
        }

        return result;
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
