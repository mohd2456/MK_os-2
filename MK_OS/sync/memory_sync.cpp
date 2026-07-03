#ifndef MK_MEMORY_SYNC_CPP
#define MK_MEMORY_SYNC_CPP

#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <algorithm>
#include <sstream>
#include <fstream>

// ===================================================================================
// MK MEMORY SYNC
// Ensures conversation memory persists across devices. When you talk to MK on
// Telegram from phone, the conversation context is available when you sit at the PC.
// Stores conversation state in JSON, syncs via device_comm.
// ===================================================================================

struct MKConversationTurn {
    std::string speaker;     // "user" or "mk"
    std::string message;
    std::time_t timestamp;
    std::string device_id;   // Which device this was said on
    std::string channel;     // "telegram", "repl", "api"
};

struct MKConversationContext {
    std::string user_id;
    std::vector<MKConversationTurn> turns;
    std::vector<std::string> active_topics;
    std::string current_mood;
    std::time_t last_interaction;
    std::string last_device;
    int turn_count;
};

struct MKMemorySyncState {
    std::string device_id;
    std::time_t last_sync;
    int turns_synced;
    bool needs_sync;
};

class MKMemorySync {
private:
    std::map<std::string, MKConversationContext> contexts;  // user_id -> context
    std::map<std::string, MKMemorySyncState> sync_states;
    std::string local_device_id;
    int max_turns_per_context;
    int max_contexts;
    std::string persistence_path;

    void trimContext(MKConversationContext& ctx) {
        if ((int)ctx.turns.size() > max_turns_per_context) {
            ctx.turns.erase(ctx.turns.begin(),
                          ctx.turns.begin() + ((int)ctx.turns.size() - max_turns_per_context));
        }
    }

public:
    MKMemorySync() : local_device_id("local_mk"), max_turns_per_context(200),
                     max_contexts(50), persistence_path("mk_memory_sync.json") {}

    // Set local device ID
    void setDeviceId(const std::string& id) { local_device_id = id; }

    // Record a conversation turn
    void addTurn(const std::string& user_id, const std::string& speaker,
                 const std::string& message, const std::string& channel = "repl") {
        MKConversationContext& ctx = contexts[user_id];
        ctx.user_id = user_id;

        MKConversationTurn turn;
        turn.speaker = speaker;
        turn.message = message;
        turn.timestamp = std::time(nullptr);
        turn.device_id = local_device_id;
        turn.channel = channel;
        ctx.turns.push_back(turn);

        ctx.last_interaction = turn.timestamp;
        ctx.last_device = local_device_id;
        ctx.turn_count++;

        trimContext(ctx);

        // Mark as needing sync
        for (auto& kv : sync_states) {
            kv.second.needs_sync = true;
        }
    }

    // Get conversation context for a user
    MKConversationContext* getContext(const std::string& user_id) {
        auto it = contexts.find(user_id);
        if (it != contexts.end()) return &it->second;
        return nullptr;
    }

    // Get recent turns for a user (for context window)
    std::vector<MKConversationTurn> getRecentTurns(const std::string& user_id,
                                                    int count = 20) const {
        auto it = contexts.find(user_id);
        if (it == contexts.end()) return {};

        const auto& turns = it->second.turns;
        int start = std::max(0, (int)turns.size() - count);
        return std::vector<MKConversationTurn>(turns.begin() + start, turns.end());
    }

    // Get the last N messages as a formatted string (for prompt context)
    std::string getContextString(const std::string& user_id, int count = 10) const {
        auto turns = getRecentTurns(user_id, count);
        std::ostringstream ss;
        for (const auto& t : turns) {
            ss << t.speaker << ": " << t.message << "\n";
        }
        return ss.str();
    }

    // Update active topics for a user
    void setTopics(const std::string& user_id, const std::vector<std::string>& topics) {
        contexts[user_id].active_topics = topics;
    }

    // Update mood
    void setMood(const std::string& user_id, const std::string& mood) {
        contexts[user_id].current_mood = mood;
    }

    // Serialize a context for sync transport
    std::string serializeContext(const std::string& user_id) const {
        auto it = contexts.find(user_id);
        if (it == contexts.end()) return "";

        const auto& ctx = it->second;
        std::ostringstream ss;
        ss << "USER:" << user_id << "\n";
        ss << "MOOD:" << ctx.current_mood << "\n";
        ss << "DEVICE:" << ctx.last_device << "\n";
        ss << "TOPICS:";
        for (size_t i = 0; i < ctx.active_topics.size(); i++) {
            if (i > 0) ss << ",";
            ss << ctx.active_topics[i];
        }
        ss << "\n";
        ss << "TURNS:" << ctx.turns.size() << "\n";
        for (const auto& t : ctx.turns) {
            ss << t.timestamp << "|" << t.speaker << "|" << t.device_id << "|"
               << t.channel << "|" << t.message << "\n";
        }
        return ss.str();
    }

    // Deserialize and merge a remote context
    void mergeRemoteContext(const std::string& data) {
        std::istringstream iss(data);
        std::string line;
        std::string user_id;
        std::string mood;
        std::vector<std::string> topics;
        std::vector<MKConversationTurn> remote_turns;

        while (std::getline(iss, line)) {
            if (line.substr(0, 5) == "USER:") {
                user_id = line.substr(5);
            } else if (line.substr(0, 5) == "MOOD:") {
                mood = line.substr(5);
            } else if (line.substr(0, 7) == "TOPICS:") {
                std::istringstream tss(line.substr(7));
                std::string topic;
                while (std::getline(tss, topic, ',')) {
                    if (!topic.empty()) topics.push_back(topic);
                }
            } else if (line.substr(0, 6) == "TURNS:") {
                // Next lines are turn data
            } else if (!line.empty() && line.find('|') != std::string::npos) {
                // Parse turn: timestamp|speaker|device|channel|message
                MKConversationTurn turn;
                std::istringstream tiss(line);
                std::string val;
                if (std::getline(tiss, val, '|')) {
                    try { turn.timestamp = static_cast<std::time_t>(std::stol(val)); } catch (...) { continue; }
                }
                std::getline(tiss, turn.speaker, '|');
                std::getline(tiss, turn.device_id, '|');
                std::getline(tiss, turn.channel, '|');
                std::getline(tiss, turn.message);
                remote_turns.push_back(turn);
            }
        }

        if (user_id.empty()) return;

        // Merge: add remote turns that don't exist locally (by timestamp)
        MKConversationContext& ctx = contexts[user_id];
        ctx.user_id = user_id;
        if (!mood.empty()) ctx.current_mood = mood;
        if (!topics.empty()) ctx.active_topics = topics;

        for (const auto& rt : remote_turns) {
            bool exists = false;
            for (const auto& lt : ctx.turns) {
                if (lt.timestamp == rt.timestamp && lt.speaker == rt.speaker &&
                    lt.message == rt.message) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                ctx.turns.push_back(rt);
            }
        }

        // Sort turns by timestamp
        std::sort(ctx.turns.begin(), ctx.turns.end(),
                  [](const MKConversationTurn& a, const MKConversationTurn& b) {
                      return a.timestamp < b.timestamp;
                  });

        trimContext(ctx);

        if (!ctx.turns.empty()) {
            ctx.last_interaction = ctx.turns.back().timestamp;
            ctx.last_device = ctx.turns.back().device_id;
        }
    }

    // Register a sync peer
    void registerPeer(const std::string& device_id) {
        MKMemorySyncState state;
        state.device_id = device_id;
        state.last_sync = 0;
        state.turns_synced = 0;
        state.needs_sync = true;
        sync_states[device_id] = state;
    }

    // Check if sync is needed
    bool needsSync(const std::string& peer_id) const {
        auto it = sync_states.find(peer_id);
        if (it == sync_states.end()) return false;
        return it->second.needs_sync;
    }

    // Mark sync complete
    void markSynced(const std::string& peer_id) {
        auto it = sync_states.find(peer_id);
        if (it != sync_states.end()) {
            it->second.last_sync = std::time(nullptr);
            it->second.needs_sync = false;
        }
    }

    // Get context count
    int contextCount() const { return static_cast<int>(contexts.size()); }

    // Get total turn count across all contexts
    int totalTurnCount() const {
        int total = 0;
        for (const auto& kv : contexts) {
            total += static_cast<int>(kv.second.turns.size());
        }
        return total;
    }

    // Save to disk
    void save() const {
        std::ofstream out(persistence_path);
        if (!out.is_open()) return;
        for (const auto& kv : contexts) {
            out << serializeContext(kv.first);
            out << "---\n";  // Context separator
        }
        out.close();
    }

    // Load from disk
    void load() {
        std::ifstream in(persistence_path);
        if (!in.is_open()) return;
        std::string content((std::istreambuf_iterator<char>(in)),
                            std::istreambuf_iterator<char>());
        in.close();

        // Split by context separator and merge each
        std::istringstream iss(content);
        std::string block;
        std::string line;
        while (std::getline(iss, line)) {
            if (line == "---") {
                if (!block.empty()) mergeRemoteContext(block);
                block.clear();
            } else {
                block += line + "\n";
            }
        }
        if (!block.empty()) mergeRemoteContext(block);
    }

    // Get summary
    std::string getSummary() const {
        std::ostringstream ss;
        ss << "=== Memory Sync ===\n";
        ss << "  Device: " << local_device_id << "\n";
        ss << "  Contexts: " << contexts.size() << "\n";
        ss << "  Total turns: " << totalTurnCount() << "\n";
        ss << "  Sync peers: " << sync_states.size() << "\n";
        for (const auto& kv : contexts) {
            ss << "  User '" << kv.first << "': " << kv.second.turns.size()
               << " turns, last on " << kv.second.last_device << "\n";
        }
        return ss.str();
    }
};

#endif // MK_MEMORY_SYNC_CPP
