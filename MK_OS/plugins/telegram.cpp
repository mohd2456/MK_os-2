#ifndef MK_TELEGRAM_CPP
#define MK_TELEGRAM_CPP

#include <iostream>
#include <string>
#include <cstdlib>
#include <map>
#include <sstream>
#include <fstream>
#include <chrono>
#include <vector>
#include <curl/curl.h>

class MKTelegram {
private:
    std::string token;

    // Rate limiting state
    std::chrono::steady_clock::time_point lastMessageTime;
    int messageCount = 0;
    static constexpr int MAX_MESSAGES_PER_MINUTE = 25;
    static constexpr int RATE_LIMIT_WINDOW_MS = 60000;

    // Retry configuration
    static constexpr int MAX_RETRIES = 3;
    static constexpr int RETRY_DELAY_MS = 1000;

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        if (!userp) return 0;
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    std::string request(const std::string& url, const std::string& postData = "") {
        std::string readBuffer;

        for (int attempt = 0; attempt < MAX_RETRIES; attempt++) {
            readBuffer.clear();
            CURL* curl = curl_easy_init();
            if (!curl) return "";

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);

            if (!postData.empty()) {
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
                curl_easy_setopt(curl, CURLOPT_POST, 1L);
            }

            CURLcode res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);

            if (res == CURLE_OK) {
                return readBuffer;
            }

            // Retry on network errors
            if (attempt < MAX_RETRIES - 1) {
                std::cerr << "[TELEGRAM] Request failed (attempt " << (attempt + 1)
                          << "/" << MAX_RETRIES << "): " << curl_easy_strerror(res) << "\n";
                // Simple sleep via busy wait (no extra includes needed)
                auto start = std::chrono::steady_clock::now();
                while (std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - start).count() < RETRY_DELAY_MS * (attempt + 1)) {
                    // wait
                }
            } else {
                std::cerr << "[TELEGRAM ERROR] Request failed after " << MAX_RETRIES
                          << " attempts: " << curl_easy_strerror(res) << "\n";
            }
        }
        return readBuffer;
    }

    // Rate limiter: returns true if message should be sent
    bool checkRateLimit() {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastMessageTime).count();

        if (elapsed > RATE_LIMIT_WINDOW_MS) {
            messageCount = 0;
            lastMessageTime = now;
        }

        if (messageCount >= MAX_MESSAGES_PER_MINUTE) {
            std::cerr << "[TELEGRAM] Rate limit reached (" << MAX_MESSAGES_PER_MINUTE
                      << " msgs/min). Dropping message.\n";
            return false;
        }

        messageCount++;
        return true;
    }

    // System status fetcher
    std::string getSystemStats() {
        std::stringstream stats;
        stats << "<b>MK-OS Status Report</b>\n\n";

        std::ifstream meminfo("/proc/meminfo");
        if (meminfo.is_open()) {
            std::string line;
            while (std::getline(meminfo, line)) {
                if (line.find("MemAvailable:") != std::string::npos) {
                    stats << "<code>" << line << "</code>\n";
                    break;
                }
            }
            meminfo.close();
        } else {
            stats << "Memory Context: Stable\n";
        }

        stats << "\nHardware Target: Native\n";
        stats << "Mode: Hybrid Reasoning Engine v2.0\n";
        return stats.str();
    }

    // URL encode helper
    std::string urlEncode(const std::string& text) {
        CURL* curl = curl_easy_init();
        if (!curl) return text;

        char* output = curl_easy_escape(curl, text.c_str(), static_cast<int>(text.length()));
        std::string encoded(output);
        curl_free(output);
        curl_easy_cleanup(curl);

        return encoded;
    }

    // Build inline keyboard JSON
    std::string buildInlineKeyboard(const std::vector<std::vector<std::pair<std::string, std::string>>>& buttons) {
        std::string json = "{\"inline_keyboard\":[";
        for (size_t row = 0; row < buttons.size(); row++) {
            json += "[";
            for (size_t col = 0; col < buttons[row].size(); col++) {
                json += "{\"text\":\"" + buttons[row][col].first +
                        "\",\"callback_data\":\"" + buttons[row][col].second + "\"}";
                if (col < buttons[row].size() - 1) json += ",";
            }
            json += "]";
            if (row < buttons.size() - 1) json += ",";
        }
        json += "]}";
        return json;
    }

public:
    MKTelegram(const std::string& botToken) : token(botToken) {
        lastMessageTime = std::chrono::steady_clock::now();
        std::cout << "[PLUGIN - TELEGRAM] Telegram bot layer initialized.\n";
    }

    // Default constructor: reads token from MK_TELEGRAM_TOKEN environment variable
    MKTelegram() {
        lastMessageTime = std::chrono::steady_clock::now();
        const char* envToken = std::getenv("MK_TELEGRAM_TOKEN");
        if (envToken && envToken[0] != '\0') {
            token = std::string(envToken);
            std::cout << "[PLUGIN - TELEGRAM] Token loaded from MK_TELEGRAM_TOKEN env var.\n";
        } else {
            std::cerr << "[PLUGIN - TELEGRAM] WARNING: MK_TELEGRAM_TOKEN not set. "
                      << "Bot will not be able to authenticate.\n";
            token = "";
        }
    }

    // Send message with HTML parse mode
    std::string sendMessage(const std::string& chatId, const std::string& message) {
        if (!checkRateLimit()) return "";

        std::string postData = "chat_id=" + chatId +
                               "&text=" + urlEncode(message) +
                               "&parse_mode=HTML";

        std::string url = "https://api.telegram.org/bot" + token + "/sendMessage";
        return request(url, postData);
    }

    // Send message with inline keyboard
    std::string sendMessageWithKeyboard(const std::string& chatId, const std::string& message,
                                         const std::vector<std::vector<std::pair<std::string, std::string>>>& buttons) {
        if (!checkRateLimit()) return "";

        std::string keyboard = buildInlineKeyboard(buttons);
        std::string postData = "chat_id=" + chatId +
                               "&text=" + urlEncode(message) +
                               "&parse_mode=HTML" +
                               "&reply_markup=" + urlEncode(keyboard);

        std::string url = "https://api.telegram.org/bot" + token + "/sendMessage";
        return request(url, postData);
    }

    std::string getUpdates(long offset = 0) {
        std::string url = "https://api.telegram.org/bot" + token + "/getUpdates";
        if (offset > 0) {
            url += "?offset=" + std::to_string(offset) + "&timeout=5";
        }
        return request(url);
    }

    bool hasToken() const { return !token.empty(); }

    void autoReply(const std::string& chatId, const std::string& msgText) {
        if (msgText == "/start") {
            // Welcome message with inline keyboard
            std::vector<std::vector<std::pair<std::string, std::string>>> buttons = {
                {{"Ask MK", "/ask"}, {"Think Deep", "/think"}},
                {{"Learn Fact", "/learn"}, {"Status", "/status"}},
                {{"Help", "/help"}}
            };
            sendMessageWithKeyboard(chatId,
                "<b>Welcome to MK-OS!</b>\n\n"
                "I'm your hybrid AI assistant. Ask me anything!\n\n"
                "Use the buttons below or type commands:",
                buttons);
        } else if (msgText == "/help") {
            sendMessage(chatId,
                "<b>MK-OS Commands:</b>\n\n"
                "/start - Boot MK\n"
                "/help - Show commands\n"
                "/ask &lt;query&gt; - Look up knowledge\n"
                "/think &lt;topic&gt; - Deep reasoning\n"
                "/learn &lt;s|r|t&gt; - Teach a fact\n"
                "/status - System diagnostics\n\n"
                "<i>Or just send a message and I'll figure it out!</i>");
        } else if (msgText == "/status") {
            sendMessage(chatId, getSystemStats());
        } else if (msgText.size() > 6 && msgText.substr(0, 6) == "/learn") {
            std::string fact = msgText.substr(6);
            // Trim leading space
            if (!fact.empty() && fact[0] == ' ') fact = fact.substr(1);
            if (fact.empty()) {
                sendMessage(chatId,
                    "<b>Usage:</b> /learn source|relation|target\n"
                    "<b>Example:</b> /learn earth|has|moon");
            } else {
                // Return the fact for the caller to process
                sendMessage(chatId,
                    "<b>Learning:</b> <code>" + fact + "</code>\n"
                    "Processing...");
            }
        } else if (msgText.size() > 7 && msgText.substr(0, 7) == "/think ") {
            std::string topic = msgText.substr(7);
            sendMessage(chatId, "Thinking about: <b>" + topic + "</b>...");
        } else if (msgText == "/think" || msgText == "/learn") {
            if (msgText == "/think") {
                sendMessage(chatId, "<b>Usage:</b> /think &lt;topic&gt;\n<b>Example:</b> /think quantum computing");
            } else {
                sendMessage(chatId, "<b>Usage:</b> /learn source|relation|target\n<b>Example:</b> /learn earth|has|moon");
            }
        } else if (msgText[0] == '/') {
            // Unknown command
            sendMessage(chatId, "Unknown command. Type /help for available commands.");
        }
        // Non-command messages are handled by the caller (routed through AI pipeline)
    }

    // Check if a message is a /learn command and extract the fact
    bool isLearnCommand(const std::string& msgText) const {
        return msgText.size() > 7 && msgText.substr(0, 7) == "/learn ";
    }

    std::string extractLearnFact(const std::string& msgText) const {
        if (isLearnCommand(msgText)) {
            return msgText.substr(7);
        }
        return "";
    }

    // Check if a message is a /think command and extract the topic
    bool isThinkCommand(const std::string& msgText) const {
        return msgText.size() > 7 && msgText.substr(0, 7) == "/think ";
    }

    std::string extractThinkTopic(const std::string& msgText) const {
        if (isThinkCommand(msgText)) {
            return msgText.substr(7);
        }
        return "";
    }

    // Check if message is a regular conversation (not a command)
    bool isConversation(const std::string& msgText) const {
        return !msgText.empty() && msgText[0] != '/';
    }
};

#endif // MK_TELEGRAM_CPP
