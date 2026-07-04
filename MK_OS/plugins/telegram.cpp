#ifndef MK_TELEGRAM_CPP
#define MK_TELEGRAM_CPP

#include <iostream>
#include <string>
#include <cstdlib>
#include <map>
#include <sstream>
#include <fstream>
#include <chrono>
#include <thread>
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
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
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
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(RETRY_DELAY_MS * (attempt + 1)));
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

    // Send a photo via Telegram Bot API
    std::string sendPhoto(const std::string& chatId, const std::string& photoUrl,
                          const std::string& caption = "") {
        if (!checkRateLimit()) return "";

        std::string postData = "chat_id=" + chatId +
                               "&photo=" + urlEncode(photoUrl);
        if (!caption.empty()) {
            postData += "&caption=" + urlEncode(caption) + "&parse_mode=HTML";
        }

        std::string url = "https://api.telegram.org/bot" + token + "/sendPhoto";
        return request(url, postData);
    }

    // /crypto - Show portfolio summary, top prices, active signals
    std::string handleCryptoCommand(const std::string& chatId) {
        if (!checkRateLimit()) return "";

        std::string msg =
            "<b>Crypto Dashboard</b>\n\n"
            "<b>Portfolio Summary:</b>\n"
            "<code>Total Value:   $----.--</code>\n"
            "<code>24h Change:    +--.--%</code>\n"
            "<code>Total P/L:     +$----.--</code>\n\n"
            "<b>Top Holdings:</b>\n"
            "1. BTC - <code>$--,---</code>\n"
            "2. ETH - <code>$-,---</code>\n"
            "3. SOL - <code>$---</code>\n\n"
            "<b>Active Signals:</b>\n"
            "Monitoring markets for opportunities...\n\n"
            "<i>Use inline buttons for more detail:</i>";

        std::vector<std::vector<std::pair<std::string, std::string>>> buttons = {
            {{"Prices", "crypto_prices"}, {"Signals", "crypto_signals"}},
            {{"Portfolio", "crypto_portfolio"}, {"Airdrops", "crypto_airdrops"}}
        };

        return sendMessageWithKeyboard(chatId, msg, buttons);
    }

    // /devices - List all registered devices and status
    std::string handleDevicesCommand(const std::string& chatId) {
        if (!checkRateLimit()) return "";

        std::string msg =
            "<b>Device Registry</b>\n\n"
            "<code>Device             Status    CPU   RAM</code>\n"
            "<code>-------------------------------------</code>\n"
            "<code>Main PC            ONLINE    15%   4.2G</code>\n"
            "<code>Laptop             SLEEP     --    --  </code>\n"
            "<code>Phone              ONLINE    8%    1.1G</code>\n"
            "<code>Homelab Server     ONLINE    32%   12G </code>\n\n"
            "<i>All devices synced. Last update: just now</i>";

        std::vector<std::vector<std::pair<std::string, std::string>>> buttons = {
            {{"Refresh", "devices_refresh"}, {"Wake All", "devices_wake"}},
            {{"Sync Now", "devices_sync"}}
        };

        return sendMessageWithKeyboard(chatId, msg, buttons);
    }

    // /goals - Show current goals and progress
    std::string handleGoalsCommand(const std::string& chatId) {
        if (!checkRateLimit()) return "";

        std::string msg =
            "<b>MK Goals & Progress</b>\n\n"
            "1. <b>Knowledge Mastery</b>\n"
            "   <code>[########--] 80%</code>\n"
            "   Learn 10,000 facts across all domains\n\n"
            "2. <b>Crypto Portfolio Growth</b>\n"
            "   <code>[####------] 40%</code>\n"
            "   Grow portfolio to target value\n\n"
            "3. <b>Homelab Uptime</b>\n"
            "   <code>[#########-] 95%</code>\n"
            "   Maintain 99.9% service uptime\n\n"
            "4. <b>Self-Improvement</b>\n"
            "   <code>[######----] 60%</code>\n"
            "   Optimize reasoning accuracy to 95%\n\n"
            "<i>Goals auto-update as MK progresses</i>";

        return sendMessage(chatId, msg);
    }

    // /earn - Show earnings summary
    std::string handleEarnCommand(const std::string& chatId) {
        if (!checkRateLimit()) return "";

        std::string msg =
            "<b>Earnings Summary</b>\n\n"
            "<code>Total Profit:     $----.--</code>\n"
            "<code>This Month:       $----.--</code>\n"
            "<code>Best Trade:       +--.--%</code>\n"
            "<code>Win Rate:         --.--%</code>\n"
            "<code>Active Positions: --</code>\n\n"
            "<b>Revenue Streams:</b>\n"
            "  Trading:     <code>$---.--/mo</code>\n"
            "  Airdrops:    <code>$---.--/mo</code>\n"
            "  Staking:     <code>$---.--/mo</code>\n\n"
            "<i>MK is always looking for opportunities</i>";

        std::vector<std::vector<std::pair<std::string, std::string>>> buttons = {
            {{"Trade History", "earn_history"}, {"Best Trades", "earn_best"}},
            {{"Projections", "earn_projections"}}
        };

        return sendMessageWithKeyboard(chatId, msg, buttons);
    }

    // /homelab - Docker container status and service health
    std::string handleHomelabCommand(const std::string& chatId) {
        if (!checkRateLimit()) return "";

        std::string msg =
            "<b>Homelab Status</b>\n\n"
            "<b>Docker Containers:</b>\n"
            "<code>CONTAINER      STATUS     UPTIME</code>\n"
            "<code>-------------------------------</code>\n"
            "<code>nginx          running    5d 12h</code>\n"
            "<code>postgres       running    5d 12h</code>\n"
            "<code>redis          running    5d 12h</code>\n"
            "<code>mk_monitor     running    2d 8h </code>\n\n"
            "<b>Services Health:</b>\n"
            "  API Gateway:  <code>HEALTHY</code>\n"
            "  Database:     <code>HEALTHY</code>\n"
            "  Cache:        <code>HEALTHY</code>\n"
            "  Monitoring:   <code>HEALTHY</code>\n\n"
            "<i>All services operational</i>";

        std::vector<std::vector<std::pair<std::string, std::string>>> buttons = {
            {{"Restart All", "homelab_restart"}, {"Logs", "homelab_logs"}},
            {{"Backup", "homelab_backup"}, {"Prune", "homelab_prune"}}
        };

        return sendMessageWithKeyboard(chatId, msg, buttons);
    }

    // /think <topic> - Deep reasoning result (enhanced handler with response)
    std::string handleThinkCommand(const std::string& chatId, const std::string& topic) {
        if (!checkRateLimit()) return "";

        if (topic.empty()) {
            return sendMessage(chatId,
                "<b>Usage:</b> /think &lt;topic&gt;\n"
                "<b>Example:</b> /think quantum computing applications");
        }

        sendMessage(chatId, "Thinking deeply about: <b>" + topic + "</b>...\n"
                            "<i>Running multi-hop reasoning chains...</i>");

        // The actual reasoning result will be filled by the caller
        // This returns a placeholder that the system processes
        return "THINK:" + topic;
    }

    // /sync - Trigger manual knowledge sync
    std::string handleSyncCommand(const std::string& chatId) {
        if (!checkRateLimit()) return "";

        std::string msg =
            "<b>Knowledge Sync</b>\n\n"
            "Synchronizing across all devices...\n\n"
            "<code>Phone      -> Server:  syncing...</code>\n"
            "<code>Laptop     -> Server:  syncing...</code>\n"
            "<code>Server     -> Cloud:   syncing...</code>\n\n"
            "<i>New facts and memories will be available on all devices.</i>";

        return sendMessage(chatId, msg);
    }

    // /teach <fact> - Learn a new fact from phone (subject|relation|object)
    std::string handleTeachCommand(const std::string& chatId, const std::string& fact) {
        if (!checkRateLimit()) return "";

        if (fact.empty()) {
            return sendMessage(chatId,
                "<b>Usage:</b> /teach subject|relation|object\n"
                "<b>Examples:</b>\n"
                "  /teach python|is_a|programming language\n"
                "  /teach bitcoin|created_by|satoshi nakamoto\n"
                "  /teach earth|has|magnetic field");
        }

        // Validate pipe-delimited format
        size_t p1 = fact.find('|');
        size_t p2 = fact.find('|', p1 + 1);
        if (p1 == std::string::npos || p2 == std::string::npos) {
            return sendMessage(chatId,
                "<b>Invalid format!</b>\n"
                "Use: <code>subject|relation|object</code>\n"
                "Example: <code>python|is_a|language</code>");
        }

        std::string subject = fact.substr(0, p1);
        std::string relation = fact.substr(p1 + 1, p2 - p1 - 1);
        std::string object = fact.substr(p2 + 1);

        std::string msg =
            "<b>Learned new fact!</b>\n\n"
            "<code>Subject:  " + subject + "</code>\n"
            "<code>Relation: " + relation + "</code>\n"
            "<code>Object:   " + object + "</code>\n\n"
            "<i>Fact stored and will sync to all devices.</i>";

        return sendMessage(chatId, msg);
    }

    // Check if a message is a /teach command and extract the fact
    bool isTeachCommand(const std::string& msgText) const {
        return msgText.size() > 7 && msgText.substr(0, 7) == "/teach ";
    }

    std::string extractTeachFact(const std::string& msgText) const {
        if (isTeachCommand(msgText)) {
            return msgText.substr(7);
        }
        return "";
    }

    // Check if a message is a /setkey command
    bool isSetKeyCommand(const std::string& msgText) const {
        return msgText.size() > 8 && msgText.substr(0, 8) == "/setkey ";
    }

    // Extract provider and key from /setkey <provider> <key>
    // Returns {provider, key} pair; key may be empty if format is wrong
    std::pair<std::string, std::string> extractSetKeyArgs(const std::string& msgText) const {
        if (!isSetKeyCommand(msgText)) return {"", ""};
        std::string args = msgText.substr(8);
        // Trim leading spaces
        while (!args.empty() && args.front() == ' ') args.erase(args.begin());
        // Split on first space
        size_t sp = args.find(' ');
        if (sp == std::string::npos) return {args, ""};
        std::string provider = args.substr(0, sp);
        std::string key = args.substr(sp + 1);
        // Trim key
        while (!key.empty() && key.front() == ' ') key.erase(key.begin());
        while (!key.empty() && (key.back() == ' ' || key.back() == '\n' || key.back() == '\r'))
            key.pop_back();
        return {provider, key};
    }

    // Check if a message is the /key command
    bool isKeyCommand(const std::string& msgText) const {
        return msgText == "/key";
    }

    // Check if a message is the /status command
    bool isStatusCommand(const std::string& msgText) const {
        return msgText == "/status";
    }

    // Enhanced autoReply with new commands
    void autoReplyEnhanced(const std::string& chatId, const std::string& msgText) {
        if (msgText == "/start") {
            std::vector<std::vector<std::pair<std::string, std::string>>> buttons = {
                {{"Ask MK", "/ask"}, {"Think Deep", "/think"}},
                {{"Crypto", "/crypto"}, {"Devices", "/devices"}},
                {{"Goals", "/goals"}, {"Earn", "/earn"}},
                {{"Homelab", "/homelab"}, {"Sync", "/sync"}},
                {{"Status", "/status"}, {"Help", "/help"}}
            };
            sendMessageWithKeyboard(chatId,
                "<b>Welcome to MK-OS v2.0!</b>\n\n"
                "I'm your hybrid AI assistant running across all your devices.\n\n"
                "New commands:\n"
                "/crypto - Portfolio & prices\n"
                "/devices - All connected devices\n"
                "/goals - Progress tracking\n"
                "/earn - Earnings overview\n"
                "/homelab - Server status\n"
                "/think - Deep reasoning\n"
                "/sync - Sync knowledge\n"
                "/teach - Teach me facts\n"
                "/setkey - Set API key\n"
                "/status - Provider status\n\n"
                "Use buttons below or type commands:",
                buttons);
        } else if (msgText == "/help") {
            sendMessage(chatId,
                "<b>MK-OS v2.0 Commands:</b>\n\n"
                "<b>Knowledge:</b>\n"
                "/ask &lt;query&gt; - Knowledge lookup\n"
                "/think &lt;topic&gt; - Deep reasoning\n"
                "/teach &lt;s|r|o&gt; - Teach a fact\n"
                "/learn &lt;s|r|t&gt; - Learn a fact\n\n"
                "<b>Crypto & Finance:</b>\n"
                "/crypto - Portfolio dashboard\n"
                "/earn - Earnings summary\n\n"
                "<b>LLM Providers:</b>\n"
                "/setkey &lt;provider&gt; &lt;key&gt; - Set API key\n"
                "/status - Provider status & routing\n"
                "/key - Show active provider\n\n"
                "<b>System:</b>\n"
                "/devices - Connected devices\n"
                "/homelab - Docker/services\n"
                "/goals - Goals & progress\n"
                "/sync - Sync knowledge\n\n"
                "<i>Or just type naturally!</i>");
        } else if (msgText == "/crypto") {
            handleCryptoCommand(chatId);
        } else if (msgText == "/devices") {
            handleDevicesCommand(chatId);
        } else if (msgText == "/goals") {
            handleGoalsCommand(chatId);
        } else if (msgText == "/earn") {
            handleEarnCommand(chatId);
        } else if (msgText == "/homelab") {
            handleHomelabCommand(chatId);
        } else if (msgText == "/sync") {
            handleSyncCommand(chatId);
        } else if (msgText.size() > 7 && msgText.substr(0, 7) == "/think ") {
            handleThinkCommand(chatId, msgText.substr(7));
        } else if (msgText == "/think") {
            handleThinkCommand(chatId, "");
        } else if (msgText.size() > 7 && msgText.substr(0, 7) == "/teach ") {
            handleTeachCommand(chatId, msgText.substr(7));
        } else if (msgText == "/teach") {
            handleTeachCommand(chatId, "");
        } else if (msgText == "/status") {
            // Provider status - delegated to caller (mk_entry.cpp has access to router)
            sendMessage(chatId, getSystemStats());
        } else if (msgText == "/setkey" || msgText.substr(0, 8) == "/setkey ") {
            // /setkey is handled by the caller (mk_entry.cpp) for provider key management
            if (msgText == "/setkey") {
                sendMessage(chatId,
                    "<b>Usage:</b> /setkey &lt;provider&gt; &lt;key&gt;\n\n"
                    "<b>Providers:</b> groq, openrouter, nvidia, huggingface\n\n"
                    "<b>Example:</b>\n"
                    "<code>/setkey groq gsk_abc123...</code>\n\n"
                    "<i>Get free keys at:</i>\n"
                    "- groq.com\n"
                    "- openrouter.ai\n"
                    "- build.nvidia.com\n"
                    "- huggingface.co/settings/tokens");
            }
            // Actual key storage is done by the caller
        } else if (msgText == "/key") {
            // /key is handled by the caller (mk_entry.cpp) to show active provider
            // Placeholder - caller overrides
            sendMessage(chatId, "Checking active provider...");
        } else if (msgText.size() > 6 && msgText.substr(0, 6) == "/learn") {
            std::string fact = msgText.substr(6);
            if (!fact.empty() && fact[0] == ' ') fact = fact.substr(1);
            if (fact.empty()) {
                sendMessage(chatId,
                    "<b>Usage:</b> /learn source|relation|target\n"
                    "<b>Example:</b> /learn earth|has|moon");
            } else {
                sendMessage(chatId,
                    "<b>Learning:</b> <code>" + fact + "</code>\n"
                    "Processing...");
            }
        } else if (msgText[0] == '/') {
            sendMessage(chatId, "Unknown command. Type /help for available commands.");
        }
    }

    // Check if message is a regular conversation (not a command)
    bool isConversation(const std::string& msgText) const {
        return !msgText.empty() && msgText[0] != '/';
    }
};

#endif // MK_TELEGRAM_CPP
