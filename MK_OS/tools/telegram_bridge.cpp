// ============================================================
// MK OS - Telegram Bridge (Complete Rewrite)
// Full-featured Telegram bot serving as MK's mobile interface.
//
// Commands: /ask, /search, /status, /learn, /weather, /time, /news, /help
//
// Compiles standalone:
//   clang++ -std=c++17 tools/telegram_bridge.cpp -o mk_telegram -lcurl
//
// Usage:
//   export MK_TELEGRAM_TOKEN="your_bot_token"
//   ./mk_telegram
// ============================================================

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <map>
#include <algorithm>
#include <functional>

// ============================================================
// HTTP Client (uses libcurl via popen for standalone mode)
// ============================================================

class TGHttpClient {
public:
    std::string get(const std::string& url) {
        std::string cmd = "curl -s --max-time 10 '" + url + "'";
        return exec_cmd(cmd);
    }

    std::string post_json(const std::string& url, const std::string& json_data) {
        std::string cmd = "curl -s --max-time 10 -X POST -H 'Content-Type: application/json' -d '"
                         + json_data + "' '" + url + "'";
        return exec_cmd(cmd);
    }

private:
    std::string exec_cmd(const std::string& cmd) {
        std::string result;
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) return "";
        char buf[4096];
        while (fgets(buf, sizeof(buf), pipe)) {
            result += buf;
        }
        pclose(pipe);
        return result;
    }
};

// ============================================================
// Minimal JSON Helper for Telegram API responses
// ============================================================

class JSONHelper {
public:
    static std::string get_string(const std::string& json, const std::string& key) {
        std::string search = "\"" + key + "\":\"";
        size_t pos = json.find(search);
        if (pos == std::string::npos) {
            search = "\"" + key + "\":";
            pos = json.find(search);
            if (pos == std::string::npos) return "";
            pos += search.size();
            while (pos < json.size() && json[pos] == ' ') pos++;
            size_t end = json.find_first_of(",}]", pos);
            return json.substr(pos, end - pos);
        }
        pos += search.size();
        size_t end = json.find("\"", pos);
        if (end == std::string::npos) return "";
        return json.substr(pos, end - pos);
    }

    static long get_long(const std::string& json, const std::string& key) {
        std::string val = get_string(json, key);
        if (val.empty()) return 0;
        try { return std::stol(val); } catch (...) { return 0; }
    }

    static std::vector<std::string> get_array_objects(const std::string& json, const std::string& key) {
        std::vector<std::string> objects;
        std::string search = "\"" + key + "\":[";
        size_t pos = json.find(search);
        if (pos == std::string::npos) return objects;
        pos += search.size();

        int depth = 0;
        size_t obj_start = pos;
        for (size_t i = pos; i < json.size(); i++) {
            if (json[i] == '{') {
                if (depth == 0) obj_start = i;
                depth++;
            } else if (json[i] == '}') {
                depth--;
                if (depth == 0) {
                    objects.push_back(json.substr(obj_start, i - obj_start + 1));
                }
            } else if (json[i] == ']' && depth == 0) {
                break;
            }
        }
        return objects;
    }

    static std::string escape_json(const std::string& text) {
        std::string escaped;
        for (char c : text) {
            if (c == '"') escaped += "\\\"";
            else if (c == '\n') escaped += "\\n";
            else if (c == '\\') escaped += "\\\\";
            else if (c == '\t') escaped += "\\t";
            else escaped += c;
        }
        return escaped;
    }
};

// ============================================================
// Knowledge Graph (standalone version for Telegram)
// ============================================================

struct KFact {
    std::string source;
    std::string relation;
    std::string target;
    float weight;
};

class TGKnowledgeGraph {
public:
    bool load(const std::string& directory) {
        std::vector<std::string> files = {
            "core_facts.mk", "lexicon.mk", "phrases.mk", "rules.mk",
            "learned_facts.mk", "coding_knowledge.mk", "system_knowledge.mk",
            "world_knowledge.mk", "personal_facts.mk"
        };
        int total = 0;
        for (const auto& file : files) {
            std::ifstream f(directory + "/" + file);
            if (!f.is_open()) continue;
            std::string line;
            while (std::getline(f, line)) {
                if (line.empty() || line[0] == '#') continue;
                auto t = parse_fact(line);
                if (!t.source.empty()) {
                    facts_.push_back(t);
                    total++;
                }
            }
        }
        std::cout << "  Knowledge loaded: " << total << " facts.\n";
        return total > 0;
    }

    std::vector<KFact> query(const std::string& term, int limit = 10) const {
        std::vector<KFact> results;
        std::string lower = term;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        for (const auto& f : facts_) {
            std::string src_lower = f.source;
            std::string tgt_lower = f.target;
            std::transform(src_lower.begin(), src_lower.end(), src_lower.begin(), ::tolower);
            std::transform(tgt_lower.begin(), tgt_lower.end(), tgt_lower.begin(), ::tolower);
            if (src_lower.find(lower) != std::string::npos ||
                tgt_lower.find(lower) != std::string::npos) {
                results.push_back(f);
                if ((int)results.size() >= limit) break;
            }
        }
        return results;
    }

    void add_fact(const std::string& src, const std::string& rel, const std::string& tgt) {
        facts_.push_back({src, rel, tgt, 1.0f});
        std::ofstream f("ai_core/hre/knowledge_files/learned_facts.mk", std::ios::app);
        if (f.is_open()) {
            f << src << "|" << rel << "|" << tgt << "|1.0\n";
        }
    }

    size_t size() const { return facts_.size(); }

private:
    std::vector<KFact> facts_;

    KFact parse_fact(const std::string& line) {
        KFact t{};
        size_t p1 = line.find('|');
        if (p1 == std::string::npos) return t;
        size_t p2 = line.find('|', p1 + 1);
        if (p2 == std::string::npos) return t;
        size_t p3 = line.find('|', p2 + 1);
        t.source = line.substr(0, p1);
        t.relation = line.substr(p1 + 1, p2 - p1 - 1);
        t.target = line.substr(p2 + 1, (p3 != std::string::npos) ? p3 - p2 - 1 : std::string::npos);
        t.weight = (p3 != std::string::npos) ? std::stof(line.substr(p3 + 1)) : 1.0f;
        return t;
    }
};

// ============================================================
// Smart Router (routes queries to best handler)
// ============================================================

class TGSmartRouter {
public:
    TGSmartRouter(TGKnowledgeGraph& kg) : kg_(kg) {}

    std::string route_query(const std::string& query) {
        // Query knowledge graph
        auto results = kg_.query(query, 5);
        if (results.empty()) {
            return "I don't have knowledge about '" + query + "' yet. "
                   "You can teach me with /learn source|relation|target";
        }
        std::string response = "Based on my knowledge:\n\n";
        for (const auto& r : results) {
            response += "* " + r.source + " " + r.relation + " " + r.target + "\n";
        }
        return response;
    }

private:
    TGKnowledgeGraph& kg_;
};

// ============================================================
// System Diagnostics (lightweight for Telegram)
// ============================================================

class TGDiagnostics {
public:
    std::string get_status() {
        std::string status;
        status += "MK OS System Status\n";
        status += "====================\n\n";
        status += "Platform: ";
#ifdef __APPLE__
        status += "macOS (Darwin)\n";
#else
        status += "Linux\n";
#endif
        // Uptime
        std::string uptime = exec("uptime 2>/dev/null | head -1");
        if (!uptime.empty()) {
            status += "Uptime: " + uptime + "\n";
        }
        // Memory
        std::string mem = exec("vm_stat 2>/dev/null | head -3 || free -h 2>/dev/null | head -2");
        if (!mem.empty()) {
            status += "\nMemory:\n" + mem + "\n";
        }
        // Disk
        std::string disk = exec("df -h / 2>/dev/null | tail -1");
        if (!disk.empty()) {
            status += "\nDisk (root): " + disk + "\n";
        }
        // Process count
        std::string procs = exec("ps aux 2>/dev/null | wc -l");
        if (!procs.empty()) {
            status += "Processes: " + procs + "\n";
        }
        // Temperature
        std::string temp = exec("sysctl -n machdep.xcpm.cpu_thermal_level 2>/dev/null");
        if (!temp.empty()) {
            status += "Thermal level: " + temp + "\n";
        }
        return status;
    }

private:
    std::string exec(const std::string& cmd) {
        std::string result;
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) return "";
        char buf[512];
        while (fgets(buf, sizeof(buf), pipe)) result += buf;
        pclose(pipe);
        while (!result.empty() && (result.back() == '\n' || result.back() == '\r'))
            result.pop_back();
        return result;
    }
};

// ============================================================
// Weather Service
// ============================================================

class TGWeatherService {
public:
    TGWeatherService(TGHttpClient& http) : http_(http) {}

    std::string get_weather(const std::string& city) {
        if (city.empty()) return "Usage: /weather <city>\nExample: /weather London";
        // Use wttr.in free API (no key needed)
        std::string url = "https://wttr.in/" + url_encode(city) + "?format=3";
        std::string result = http_.get(url);
        if (result.empty()) {
            return "Could not fetch weather data. Try again later.";
        }
        return "Weather: " + result;
    }

private:
    TGHttpClient& http_;

    std::string url_encode(const std::string& s) {
        std::string encoded;
        for (char c : s) {
            if (c == ' ') encoded += "%20";
            else encoded += c;
        }
        return encoded;
    }
};

// ============================================================
// Time Service
// ============================================================

class TGTimeService {
public:
    std::string get_time(const std::string& timezone) {
        if (timezone.empty()) {
            // Return local time
            auto now = std::chrono::system_clock::now();
            auto t = std::chrono::system_clock::to_time_t(now);
            char buf[64];
            std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S %Z", std::localtime(&t));
            return std::string("Current time: ") + buf;
        }
        // Try to get time for specific timezone using date command
        std::string cmd = "TZ='" + timezone + "' date '+%Y-%m-%d %H:%M:%S %Z' 2>/dev/null";
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) return "Could not determine time for: " + timezone;
        char buf[128];
        std::string result;
        while (fgets(buf, sizeof(buf), pipe)) result += buf;
        pclose(pipe);
        if (result.empty()) return "Unknown timezone: " + timezone;
        return "Time in " + timezone + ": " + result;
    }
};

// ============================================================
// News Service
// ============================================================

class TGNewsService {
public:
    TGNewsService(TGHttpClient& http) : http_(http) {}

    std::string get_tech_news() {
        // Use a free news API or fallback
        std::string url = "https://hacker-news.firebaseio.com/v0/topstories.json?print=pretty";
        std::string response = http_.get(url);
        if (response.empty()) {
            return "Could not fetch news. Try again later.";
        }
        // Parse first few story IDs
        std::vector<std::string> ids;
        std::string current_id;
        for (char c : response) {
            if (c >= '0' && c <= '9') {
                current_id += c;
            } else if (!current_id.empty()) {
                ids.push_back(current_id);
                current_id.clear();
                if (ids.size() >= 5) break;
            }
        }

        std::string news = "Top Tech Headlines:\n\n";
        int count = 1;
        for (const auto& id : ids) {
            std::string story_url = "https://hacker-news.firebaseio.com/v0/item/" + id + ".json";
            std::string story = http_.get(story_url);
            std::string title = JSONHelper::get_string(story, "title");
            if (!title.empty()) {
                news += std::to_string(count++) + ". " + title + "\n";
            }
        }
        if (count == 1) {
            return "Could not parse news headlines.";
        }
        return news;
    }

private:
    TGHttpClient& http_;
};

// ============================================================
// Internet Search (using DuckDuckGo instant answer API)
// ============================================================

class TGSearchService {
public:
    TGSearchService(TGHttpClient& http) : http_(http) {}

    std::string search(const std::string& query) {
        if (query.empty()) return "Usage: /search <query>";
        std::string encoded = url_encode(query);
        std::string url = "https://api.duckduckgo.com/?q=" + encoded + "&format=json&no_html=1";
        std::string response = http_.get(url);
        if (response.empty()) {
            return "Search failed. Try again later.";
        }
        // Extract abstract
        std::string abstract = JSONHelper::get_string(response, "Abstract");
        std::string abstract_src = JSONHelper::get_string(response, "AbstractSource");
        std::string abstract_url = JSONHelper::get_string(response, "AbstractURL");

        if (!abstract.empty()) {
            std::string result = "Search: " + query + "\n\n";
            result += abstract + "\n\n";
            if (!abstract_src.empty()) result += "Source: " + abstract_src + "\n";
            if (!abstract_url.empty()) result += "URL: " + abstract_url + "\n";
            return result;
        }
        // Try related topics
        std::string heading = JSONHelper::get_string(response, "Heading");
        if (!heading.empty()) {
            return "Found: " + heading + "\nTry a more specific query for detailed results.";
        }
        return "No instant results for '" + query + "'. Try rephrasing your search.";
    }

private:
    TGHttpClient& http_;

    std::string url_encode(const std::string& s) {
        std::string encoded;
        for (char c : s) {
            if (c == ' ') encoded += "+";
            else if (c == '&') encoded += "%26";
            else if (c == '=') encoded += "%3D";
            else if (c == '?') encoded += "%3F";
            else encoded += c;
        }
        return encoded;
    }
};

// ============================================================
// Main Telegram Bot
// ============================================================

static std::atomic<bool> g_running{true};

void tg_signal_handler(int sig) {
    (void)sig;
    g_running = false;
}

class MKTelegramBot {
public:
    bool init(const std::string& token, const std::string& knowledge_dir) {
        token_ = token;
        base_url_ = "https://api.telegram.org/bot" + token_;
        knowledge_dir_ = knowledge_dir;

        // Load knowledge
        if (!kg_.load(knowledge_dir)) {
            std::cerr << "  Warning: No knowledge files loaded.\n";
        }

        // Verify bot connection
        std::string response = http_.get(base_url_ + "/getMe");
        if (response.find("\"ok\":true") == std::string::npos) {
            std::cerr << "  Error: Failed to connect to Telegram API.\n";
            std::cerr << "  Check your MK_TELEGRAM_TOKEN environment variable.\n";
            return false;
        }

        std::string bot_name = JSONHelper::get_string(response, "username");
        std::cout << "  Connected as: @" << bot_name << "\n";
        std::cout << "  Knowledge base: " << kg_.size() << " facts\n";
        return true;
    }

    void run() {
        std::cout << "  Polling for messages (long-polling)...\n\n";
        while (g_running) {
            poll_updates();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

private:
    std::string token_;
    std::string base_url_;
    std::string knowledge_dir_;
    TGHttpClient http_;
    TGKnowledgeGraph kg_;
    long last_update_id_ = 0;

    void poll_updates() {
        std::string url = base_url_ + "/getUpdates?offset="
                        + std::to_string(last_update_id_ + 1)
                        + "&timeout=15&allowed_updates=[\"message\"]";
        std::string response = http_.get(url);
        if (response.empty() || response.find("\"ok\":true") == std::string::npos) return;

        auto updates = JSONHelper::get_array_objects(response, "result");
        for (const auto& update : updates) {
            long update_id = JSONHelper::get_long(update, "update_id");
            if (update_id > last_update_id_) last_update_id_ = update_id;

            std::string text = JSONHelper::get_string(update, "text");
            std::string chat_id;

            // Extract chat_id from nested chat object
            size_t chat_pos = update.find("\"chat\":");
            if (chat_pos != std::string::npos) {
                std::string chat_section = update.substr(chat_pos);
                chat_id = JSONHelper::get_string(chat_section, "id");
            }
            if (text.empty() || chat_id.empty()) continue;

            std::cout << "  << " << text << "\n";
            std::string reply = handle_message(text);
            send_message(chat_id, reply);
            std::cout << "  >> " << reply.substr(0, 80) << (reply.size() > 80 ? "..." : "") << "\n\n";
        }
    }

    std::string handle_message(const std::string& text) {
        if (text.empty()) return "Empty message received.";
        if (text[0] == '/') return handle_command(text);
        // Default: route through smart router (ask)
        TGSmartRouter router(kg_);
        return router.route_query(text);
    }

    std::string handle_command(const std::string& text) {
        std::string cmd, args;
        size_t space = text.find(' ');
        if (space != std::string::npos) {
            cmd = text.substr(0, space);
            args = text.substr(space + 1);
        } else {
            cmd = text;
        }
        // Remove @botname suffix from command
        size_t at_pos = cmd.find('@');
        if (at_pos != std::string::npos) cmd = cmd.substr(0, at_pos);

        if (cmd == "/start" || cmd == "/help") return cmd_help();
        if (cmd == "/ask") return cmd_ask(args);
        if (cmd == "/search") return cmd_search(args);
        if (cmd == "/status") return cmd_status();
        if (cmd == "/learn") return cmd_learn(args);
        if (cmd == "/weather") return cmd_weather(args);
        if (cmd == "/time") return cmd_time(args);
        if (cmd == "/news") return cmd_news();

        return "Unknown command: " + cmd + "\nType /help for available commands.";
    }

    // --- Command Handlers ---

    std::string cmd_help() {
        return "MK OS - Telegram Interface\n"
               "===========================\n\n"
               "Available commands:\n\n"
               "/ask <question> - Ask MK anything (routes through accuracy pipeline)\n"
               "/search <query> - Internet search with cited results\n"
               "/status - System diagnostics and health\n"
               "/learn source|relation|target - Teach MK a new fact\n"
               "/weather <city> - Real-time weather report\n"
               "/time [timezone] - Current time (optional timezone)\n"
               "/news - Latest tech headlines\n"
               "/help - Show this help message\n\n"
               "You can also send plain text and MK will try to answer from its knowledge base.";
    }

    std::string cmd_ask(const std::string& args) {
        if (args.empty()) return "Usage: /ask <your question>\nExample: /ask what is Python?";
        TGSmartRouter router(kg_);
        std::string answer = router.route_query(args);
        return "Question: " + args + "\n\n" + answer;
    }

    std::string cmd_search(const std::string& args) {
        TGSearchService search(http_);
        return search.search(args);
    }

    std::string cmd_status() {
        TGDiagnostics diag;
        std::string status = diag.get_status();
        status += "\nKnowledge base: " + std::to_string(kg_.size()) + " facts\n";
        status += "Bot status: Active\n";
        return status;
    }

    std::string cmd_learn(const std::string& args) {
        if (args.empty()) {
            return "Usage: /learn source|relation|target\n"
                   "Example: /learn python|is_a|programming language\n"
                   "Example: /learn sun|has_property|hot";
        }
        size_t p1 = args.find('|');
        size_t p2 = args.find('|', p1 + 1);
        if (p1 == std::string::npos || p2 == std::string::npos) {
            return "Format error. Use pipe-separated values:\n"
                   "/learn source|relation|target";
        }
        std::string source = args.substr(0, p1);
        std::string relation = args.substr(p1 + 1, p2 - p1 - 1);
        std::string target = args.substr(p2 + 1);

        if (source.empty() || relation.empty() || target.empty()) {
            return "All three fields must be non-empty.";
        }

        kg_.add_fact(source, relation, target);
        return "Learned: " + source + " [" + relation + "] " + target + "\n"
               "Knowledge base now has " + std::to_string(kg_.size()) + " facts.";
    }

    std::string cmd_weather(const std::string& args) {
        TGWeatherService weather(http_);
        return weather.get_weather(args);
    }

    std::string cmd_time(const std::string& args) {
        TGTimeService time_svc;
        return time_svc.get_time(args);
    }

    std::string cmd_news() {
        TGNewsService news(http_);
        return news.get_tech_news();
    }

    // --- Send Message ---

    void send_message(const std::string& chat_id, const std::string& text) {
        std::string escaped = JSONHelper::escape_json(text);
        // Truncate if too long for Telegram (4096 char limit)
        std::string send_text = escaped;
        if (send_text.size() > 4000) {
            send_text = send_text.substr(0, 4000) + "\\n[truncated]";
        }
        std::string payload = "{\"chat_id\":" + chat_id
                            + ",\"text\":\"" + send_text
                            + "\",\"parse_mode\":\"Markdown\"}";
        http_.post_json(base_url_ + "/sendMessage", payload);
    }
};

// ============================================================
// Main Entry Point
// ============================================================

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;

    std::signal(SIGINT, tg_signal_handler);
    std::signal(SIGTERM, tg_signal_handler);

    std::cout << "\n";
    std::cout << "  +-----------------------------------------+\n";
    std::cout << "  |  MK OS - Telegram Bridge v2.0           |\n";
    std::cout << "  |  Full-featured mobile interface          |\n";
    std::cout << "  |  Commands: /ask /search /status /learn   |\n";
    std::cout << "  |            /weather /time /news /help    |\n";
    std::cout << "  +-----------------------------------------+\n";
    std::cout << "\n";

    // Get bot token
    const char* token_env = std::getenv("MK_TELEGRAM_TOKEN");
    if (!token_env || std::string(token_env).empty()) {
        std::cerr << "  Error: MK_TELEGRAM_TOKEN not set.\n";
        std::cerr << "  Usage:\n";
        std::cerr << "    export MK_TELEGRAM_TOKEN=\"your_bot_token\"\n";
        std::cerr << "    ./mk_telegram\n";
        return 1;
    }
    std::string token(token_env);

    // Knowledge directory
    std::string knowledge_dir = "ai_core/hre/knowledge_files";
    if (argc > 1) knowledge_dir = argv[1];

    // Initialize and run
    MKTelegramBot bot;
    if (!bot.init(token, knowledge_dir)) {
        return 1;
    }

    bot.run();

    std::cout << "\n  MK Telegram Bridge stopped gracefully.\n";
    return 0;
}
