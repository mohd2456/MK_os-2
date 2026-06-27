// ============================================================
// MK OS - Telegram Bridge
// Connects Telegram Bot API to the HRE Brain.
// Polls for messages, routes them through reasoning, responds.
//
// Compiles standalone:
//   g++ -std=c++17 tools/telegram_bridge.cpp -o mk_telegram -lcurl
//
// Usage:
//   export MK_TELEGRAM_TOKEN="your_bot_token"
//   ./mk_telegram
//
// Or managed by MKDaemon as a background service.
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
#include <regex>
#include <functional>

// ============================================================
// Minimal HTTP (uses libcurl via popen for standalone mode)
// In linked mode, uses MKHTTP class from network/http.cpp
// ============================================================

class TelegramHTTP {
public:
    std::string get(const std::string& url) {
        std::string cmd = "curl -s '" + url + "'";
        return exec_cmd(cmd);
    }
    
    std::string post(const std::string& url, const std::string& data) {
        std::string cmd = "curl -s -X POST -H 'Content-Type: application/json' -d '" 
                         + data + "' '" + url + "'";
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
// Minimal JSON Parser (no external deps)
// Extracts fields from Telegram API responses
// ============================================================

class SimpleJSON {
public:
    static std::string get_string(const std::string& json, const std::string& key) {
        std::string search = "\"" + key + "\":\"";
        size_t pos = json.find(search);
        if (pos == std::string::npos) {
            // Try without quotes (for numbers)
            search = "\"" + key + "\":";
            pos = json.find(search);
            if (pos == std::string::npos) return "";
            pos += search.size();
            size_t end = json.find_first_of(",}]", pos);
            return json.substr(pos, end - pos);
        }
        pos += search.size();
        size_t end = json.find("\"", pos);
        return json.substr(pos, end - pos);
    }
    
    static long get_long(const std::string& json, const std::string& key) {
        std::string val = get_string(json, key);
        return val.empty() ? 0 : std::stol(val);
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
};

// ============================================================
// Thermal Monitor (standalone version)
// ============================================================

class ThermalCheck {
public:
    bool is_throttled() {
        #ifdef __APPLE__
        FILE* pipe = popen("sysctl -n machdep.xcpm.cpu_thermal_level 2>/dev/null", "r");
        if (pipe) {
            char buf[32];
            if (fgets(buf, sizeof(buf), pipe)) {
                pclose(pipe);
                int level = std::atoi(buf);
                temperature_ = 40 + (level * 15);
                return level > 3; // Throttle at level 4+
            }
            pclose(pipe);
        }
        #endif
        temperature_ = 55;
        return false;
    }
    
    int temperature() const { return temperature_; }

private:
    int temperature_ = 55;
};

// ============================================================
// Knowledge Graph (minimal standalone version)
// ============================================================

struct KnowledgeTriple {
    std::string source;
    std::string relation;
    std::string target;
    float weight;
};

class KnowledgeGraph {
public:
    bool load(const std::string& directory) {
        std::vector<std::string> files = {
            "core_facts.mk", "lexicon.mk", "phrases.mk", "rules.mk",
            "learned_facts.mk", "coding_knowledge.mk", "system_knowledge.mk"
        };
        
        int total = 0;
        for (const auto& file : files) {
            std::ifstream f(directory + "/" + file);
            if (!f.is_open()) continue;
            std::string line;
            while (std::getline(f, line)) {
                if (line.empty() || line[0] == '#') continue;
                auto t = parse(line);
                if (!t.source.empty()) {
                    triples_.push_back(t);
                    total++;
                }
            }
        }
        std::cout << "  Knowledge: " << total << " facts loaded.\n";
        return total > 0;
    }
    
    std::vector<KnowledgeTriple> query(const std::string& term) const {
        std::vector<KnowledgeTriple> results;
        std::string lower_term = term;
        std::transform(lower_term.begin(), lower_term.end(), lower_term.begin(), ::tolower);
        
        for (const auto& t : triples_) {
            if (t.source.find(lower_term) != std::string::npos ||
                t.target.find(lower_term) != std::string::npos) {
                results.push_back(t);
                if (results.size() >= 10) break; // Limit for Telegram response size
            }
        }
        return results;
    }
    
    void add_fact(const std::string& source, const std::string& relation, 
                  const std::string& target, float weight = 1.0f) {
        triples_.push_back({source, relation, target, weight});
    }
    
    size_t size() const { return triples_.size(); }

private:
    std::vector<KnowledgeTriple> triples_;
    
    KnowledgeTriple parse(const std::string& line) {
        KnowledgeTriple t{};
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
// HRE Brain (standalone reasoning for Telegram)
// ============================================================

class HREBrainLite {
public:
    void init(const std::string& knowledge_dir) {
        knowledge_.load(knowledge_dir);
    }
    
    std::string process(const std::string& input) {
        // Query knowledge graph
        auto results = knowledge_.query(input);
        
        if (results.empty()) {
            return "I don't have knowledge about that yet. Teach me with /learn source|relation|target";
        }
        
        // Compose response from knowledge
        std::string response;
        for (const auto& r : results) {
            response += r.source + " " + r.relation + " " + r.target + "\n";
        }
        return response;
    }
    
    std::string deep_think(const std::string& topic) {
        auto results = knowledge_.query(topic);
        if (results.empty()) return "No deep knowledge found for: " + topic;
        
        std::string chain = "Deep reasoning on '" + topic + "':\n";
        std::string current = topic;
        
        for (int depth = 0; depth < 5; depth++) {
            auto facts = knowledge_.query(current);
            if (facts.empty()) break;
            chain += "  [" + std::to_string(depth + 1) + "] " 
                  + facts[0].source + " → " + facts[0].relation + " → " + facts[0].target + "\n";
            current = facts[0].target;
        }
        return chain;
    }
    
    std::string generate_code(const std::string& request) {
        auto results = knowledge_.query(request);
        std::string code = "// MK Code Generation\n// Request: " + request + "\n\n";
        for (const auto& r : results) {
            if (r.relation == "syntax" || r.relation == "pattern" || r.relation == "example") {
                code += "// " + r.source + ": " + r.target + "\n";
            }
        }
        return code;
    }
    
    void learn(const std::string& source, const std::string& relation, const std::string& target) {
        knowledge_.add_fact(source, relation, target, 1.0f);
        // Also persist to disk
        std::ofstream f("ai_core/hre/knowledge_files/learned_facts.mk", std::ios::app);
        if (f.is_open()) {
            f << source << "|" << relation << "|" << target << "|1.0\n";
            f.close();
        }
    }
    
    size_t knowledge_size() const { return knowledge_.size(); }

private:
    KnowledgeGraph knowledge_;
};

// ============================================================
// Telegram Bridge
// ============================================================

static std::atomic<bool> g_running{true};

void signal_handler(int sig) {
    (void)sig;
    g_running = false;
}

class TelegramBridge {
public:
    bool init(const std::string& token, const std::string& knowledge_dir) {
        token_ = token;
        base_url_ = "https://api.telegram.org/bot" + token_;
        
        // Initialize brain
        brain_.init(knowledge_dir);
        
        // Verify bot connection
        std::string response = http_.get(base_url_ + "/getMe");
        if (response.find("\"ok\":true") == std::string::npos) {
            std::cerr << "  Error: Failed to connect to Telegram API.\n";
            std::cerr << "  Check your MK_TELEGRAM_TOKEN.\n";
            return false;
        }
        
        std::string bot_name = SimpleJSON::get_string(response, "username");
        std::cout << "  Connected as: @" << bot_name << "\n";
        return true;
    }
    
    void run() {
        std::cout << "  Polling for messages...\n\n";
        
        while (g_running) {
            // Thermal check
            if (thermal_.is_throttled()) {
                std::cout << "  ⚠ Thermal throttle (" << thermal_.temperature() << "°C). Pausing...\n";
                std::this_thread::sleep_for(std::chrono::seconds(30));
                continue;
            }
            
            // Poll for updates
            poll_updates();
            
            // Sleep between polls (Telegram rate limit friendly)
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

private:
    std::string token_;
    std::string base_url_;
    TelegramHTTP http_;
    HREBrainLite brain_;
    ThermalCheck thermal_;
    long last_update_id_ = 0;
    
    void poll_updates() {
        std::string url = base_url_ + "/getUpdates?offset=" 
                        + std::to_string(last_update_id_ + 1)
                        + "&timeout=10";
        
        std::string response = http_.get(url);
        if (response.empty() || response.find("\"ok\":true") == std::string::npos) {
            return;
        }
        
        auto updates = SimpleJSON::get_array_objects(response, "result");
        
        for (const auto& update : updates) {
            long update_id = SimpleJSON::get_long(update, "update_id");
            if (update_id > last_update_id_) {
                last_update_id_ = update_id;
            }
            
            // Extract message
            std::string text = SimpleJSON::get_string(update, "text");
            std::string chat_id = SimpleJSON::get_string(update, "id");
            
            // Try to get chat_id from the message object
            size_t chat_pos = update.find("\"chat\":");
            if (chat_pos != std::string::npos) {
                std::string chat_section = update.substr(chat_pos);
                chat_id = SimpleJSON::get_string(chat_section, "id");
            }
            
            if (text.empty() || chat_id.empty()) continue;
            
            std::cout << "  ← " << text << "\n";
            
            // Process message
            std::string reply = process_message(text);
            
            // Send reply
            send_message(chat_id, reply);
            std::cout << "  → " << reply.substr(0, 80) << (reply.size() > 80 ? "..." : "") << "\n\n";
        }
    }
    
    std::string process_message(const std::string& text) {
        // Check thermal first
        if (thermal_.is_throttled()) {
            return "MK is cooling down, try again in a minute. (Temp: " 
                   + std::to_string(thermal_.temperature()) + "°C)";
        }
        
        // Handle commands
        if (text[0] == '/') {
            return handle_command(text);
        }
        
        // Natural language → HRE Brain
        return brain_.process(text);
    }
    
    std::string handle_command(const std::string& text) {
        if (text == "/start") {
            return "MK OS Telegram Bridge active.\n"
                   "Commands:\n"
                   "/status - System stats\n"
                   "/think <topic> - Deep reasoning\n"
                   "/learn source|relation|target - Add knowledge\n"
                   "/build <request> - Generate code\n"
                   "/help - Show commands";
        }
        
        if (text == "/help") {
            return "MK Commands:\n"
                   "/status - System status & stats\n"
                   "/think <topic> - Force deep multi-hop reasoning\n"
                   "/learn S|R|T - Teach MK a new fact\n"
                   "/build <desc> - Generate code\n"
                   "/temp - Thermal status";
        }
        
        if (text == "/status") {
            std::string status = "MK OS Status:\n";
            status += "Platform: ";
            #ifdef __APPLE__
            status += "macOS (Darwin)\n";
            #else
            status += "Linux\n";
            #endif
            status += "Temperature: " + std::to_string(thermal_.temperature()) + "°C\n";
            status += "Throttled: " + std::string(thermal_.is_throttled() ? "YES" : "No") + "\n";
            status += "Knowledge: " + std::to_string(brain_.knowledge_size()) + " facts\n";
            status += "Uptime: active";
            return status;
        }
        
        if (text == "/temp") {
            thermal_.is_throttled(); // refresh
            return "Temperature: " + std::to_string(thermal_.temperature()) + "°C"
                   + (thermal_.is_throttled() ? " [THROTTLED]" : " [OK]");
        }
        
        if (text.substr(0, 6) == "/think") {
            std::string topic = (text.size() > 7) ? text.substr(7) : "";
            if (topic.empty()) return "Usage: /think <topic>";
            return brain_.deep_think(topic);
        }
        
        if (text.substr(0, 6) == "/learn") {
            std::string fact = (text.size() > 7) ? text.substr(7) : "";
            if (fact.empty()) return "Usage: /learn source|relation|target";
            
            // Parse the triple
            size_t p1 = fact.find('|');
            size_t p2 = fact.find('|', p1 + 1);
            if (p1 == std::string::npos || p2 == std::string::npos) {
                return "Format error. Use: /learn source|relation|target\nExample: /learn python|is_a|programming language";
            }
            
            std::string source = fact.substr(0, p1);
            std::string relation = fact.substr(p1 + 1, p2 - p1 - 1);
            std::string target = fact.substr(p2 + 1);
            
            brain_.learn(source, relation, target);
            return "Learned: " + source + " " + relation + " " + target;
        }
        
        if (text.substr(0, 6) == "/build") {
            std::string request = (text.size() > 7) ? text.substr(7) : "";
            if (request.empty()) return "Usage: /build <description>\nExample: /build python flask hello world";
            return brain_.generate_code(request);
        }
        
        return "Unknown command. Try /help";
    }
    
    void send_message(const std::string& chat_id, const std::string& text) {
        // Escape special characters for JSON
        std::string escaped;
        for (char c : text) {
            if (c == '"') escaped += "\\\"";
            else if (c == '\n') escaped += "\\n";
            else if (c == '\\') escaped += "\\\\";
            else escaped += c;
        }
        
        std::string payload = "{\"chat_id\":" + chat_id 
                            + ",\"text\":\"" + escaped + "\"}";
        
        http_.post(base_url_ + "/sendMessage", payload);
    }
};

// ============================================================
// Main
// ============================================================

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    std::cout << R"(
  ╔═══════════════════════════════════╗
  ║   MK Telegram Bridge             ║
  ║   HRE Brain ↔ Telegram Bot API   ║
  ╚═══════════════════════════════════╝
)" << std::endl;
    
    // Get bot token from environment
    const char* token_env = std::getenv("MK_TELEGRAM_TOKEN");
    if (!token_env || std::string(token_env).empty()) {
        std::cerr << "  Error: MK_TELEGRAM_TOKEN not set.\n";
        std::cerr << "  Usage: export MK_TELEGRAM_TOKEN=\"your_bot_token\"\n";
        std::cerr << "         ./mk_telegram\n";
        return 1;
    }
    std::string token(token_env);
    
    // Knowledge directory (relative to MK_OS root or absolute)
    std::string knowledge_dir = "ai_core/hre/knowledge_files";
    if (argc > 1) {
        knowledge_dir = argv[1];
    }
    
    // Initialize bridge
    TelegramBridge bridge;
    if (!bridge.init(token, knowledge_dir)) {
        return 1;
    }
    
    // Run polling loop
    bridge.run();
    
    std::cout << "\n  MK Telegram Bridge stopped.\n";
    return 0;
}
