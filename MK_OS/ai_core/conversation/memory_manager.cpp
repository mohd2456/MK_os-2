#ifndef MK_MEMORY_MANAGER_CPP
#define MK_MEMORY_MANAGER_CPP

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <ctime>

// ===========================================================================
// MK MEMORY MANAGER - Long-term Conversation Memory
// ===========================================================================
// Stores user preferences, conversation history summaries, named entities,
// user skill level estimates, and favorite topics. Persists to disk.
// Remembers user name, preferences, and past interactions across sessions.
// ===========================================================================

// Conversation summary entry
struct MKConversationSummary {
    std::string timestamp;
    std::string topic;
    std::string summary;
    int turn_count;
    float satisfaction_score;  // estimated user satisfaction
};

// A fact learned about the user
struct MKUserFact {
    std::string key;        // e.g., 'name', 'language', 'preference'
    std::string value;
    std::string source;     // which conversation taught this
    std::string timestamp;
    int confidence;         // 1-10 how sure we are
};

// Topic history entry
struct MKTopicHistory {
    std::string topic;
    int times_discussed;
    std::string last_discussed;
    float user_interest_level;  // 0-1
};

// Complete user profile
struct MKUserProfile {
    std::string user_name;
    std::string preferred_language;  // programming language
    std::string skill_level;         // beginner, intermediate, advanced
    float expertise_score;           // 0.0 to 1.0
    std::vector<MKUserFact> facts;
    std::vector<MKConversationSummary> conversation_history;
    std::vector<MKTopicHistory> topic_history;
    std::unordered_map<std::string, std::string> preferences;
    int total_interactions;
    std::string first_seen;
    std::string last_seen;
};

class MKMemoryManager {
private:
    MKUserProfile profile;
    std::string storage_path;
    bool loaded;

    // Get current timestamp as string
    std::string getCurrentTimestamp() {
        time_t now = time(nullptr);
        char buf[64];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
        return std::string(buf);
    }

    // Serialize profile to file (simple key-value format)
    void saveToFile() {
        std::ofstream out(storage_path);
        if (!out.is_open()) return;

        out << "[profile]\n";
        out << "user_name=" << profile.user_name << "\n";
        out << "preferred_language=" << profile.preferred_language << "\n";
        out << "skill_level=" << profile.skill_level << "\n";
        out << "expertise_score=" << profile.expertise_score << "\n";
        out << "total_interactions=" << profile.total_interactions << "\n";
        out << "first_seen=" << profile.first_seen << "\n";
        out << "last_seen=" << profile.last_seen << "\n";

        out << "\n[preferences]\n";
        for (const auto& [key, value] : profile.preferences) {
            out << key << "=" << value << "\n";
        }

        out << "\n[facts]\n";
        for (const auto& fact : profile.facts) {
            out << fact.key << "|" << fact.value << "|" << fact.confidence
                << "|" << fact.timestamp << "\n";
        }

        out << "\n[topics]\n";
        for (const auto& topic : profile.topic_history) {
            out << topic.topic << "|" << topic.times_discussed << "|"
                << topic.last_discussed << "|" << topic.user_interest_level << "\n";
        }

        out << "\n[conversations]\n";
        for (const auto& conv : profile.conversation_history) {
            out << conv.timestamp << "|" << conv.topic << "|"
                << conv.turn_count << "|" << conv.satisfaction_score << "\n";
        }
        out.close();
    }

    // Load profile from file
    void loadFromFile() {
        std::ifstream in(storage_path);
        if (!in.is_open()) return;

        std::string line, section;
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            if (line[0] == '[') {
                section = line.substr(1, line.find(']') - 1);
                continue;
            }

            if (section == "profile") {
                size_t eq = line.find('=');
                if (eq == std::string::npos) continue;
                std::string key = line.substr(0, eq);
                std::string val = line.substr(eq + 1);
                if (key == "user_name") profile.user_name = val;
                else if (key == "preferred_language") profile.preferred_language = val;
                else if (key == "skill_level") profile.skill_level = val;
                else if (key == "expertise_score") profile.expertise_score = std::stof(val);
                else if (key == "total_interactions") profile.total_interactions = std::stoi(val);
                else if (key == "first_seen") profile.first_seen = val;
                else if (key == "last_seen") profile.last_seen = val;
            }
            else if (section == "preferences") {
                size_t eq = line.find('=');
                if (eq != std::string::npos)
                    profile.preferences[line.substr(0, eq)] = line.substr(eq + 1);
            }
            else if (section == "facts") {
                std::stringstream ss(line);
                MKUserFact fact;
                std::getline(ss, fact.key, '|');
                std::getline(ss, fact.value, '|');
                std::string conf; std::getline(ss, conf, '|');
                if (!conf.empty()) fact.confidence = std::stoi(conf);
                std::getline(ss, fact.timestamp, '|');
                profile.facts.push_back(fact);
            }
            else if (section == "topics") {
                std::stringstream ss(line);
                MKTopicHistory th;
                std::getline(ss, th.topic, '|');
                std::string count; std::getline(ss, count, '|');
                if (!count.empty()) th.times_discussed = std::stoi(count);
                std::getline(ss, th.last_discussed, '|');
                std::string interest; std::getline(ss, interest, '|');
                if (!interest.empty()) th.user_interest_level = std::stof(interest);
                profile.topic_history.push_back(th);
            }
        }
        loaded = true;
    }

public:
    MKMemoryManager() : loaded(false) {
        storage_path = "mk_memory.dat";
        profile.total_interactions = 0;
        profile.expertise_score = 0.5f;
        profile.skill_level = "intermediate";
    }

    // Initialize and load from disk
    void init(const std::string& path = "mk_memory.dat") {
        storage_path = path;
        loadFromFile();
        if (!loaded) {
            profile.first_seen = getCurrentTimestamp();
        }
        profile.last_seen = getCurrentTimestamp();
    }

    // Remember the user's name
    void setUserName(const std::string& name) {
        profile.user_name = name;
        addFact("name", name, 10);
        save();
    }

    std::string getUserName() const { return profile.user_name; }

    // Add a fact about the user
    void addFact(const std::string& key, const std::string& value, int confidence = 7) {
        // Update existing fact or add new one
        for (auto& fact : profile.facts) {
            if (fact.key == key) {
                fact.value = value;
                fact.confidence = confidence;
                fact.timestamp = getCurrentTimestamp();
                save();
                return;
            }
        }
        profile.facts.push_back({key, value, "conversation", getCurrentTimestamp(), confidence});
        save();
    }

    // Get a fact about the user
    std::string getFact(const std::string& key) const {
        for (const auto& fact : profile.facts) {
            if (fact.key == key) return fact.value;
        }
        return "";
    }

    // Set a preference
    void setPreference(const std::string& key, const std::string& value) {
        profile.preferences[key] = value;
        save();
    }

    std::string getPreference(const std::string& key) const {
        auto it = profile.preferences.find(key);
        return (it != profile.preferences.end()) ? it->second : "";
    }

    // Record a topic being discussed
    void recordTopic(const std::string& topic) {
        for (auto& th : profile.topic_history) {
            if (th.topic == topic) {
                th.times_discussed++;
                th.last_discussed = getCurrentTimestamp();
                th.user_interest_level = std::min(1.0f, th.user_interest_level + 0.1f);
                save();
                return;
            }
        }
        profile.topic_history.push_back({topic, 1, getCurrentTimestamp(), 0.5f});
        save();
    }

    // Record conversation summary
    void recordConversation(const std::string& topic, const std::string& summary, int turns) {
        MKConversationSummary cs;
        cs.timestamp = getCurrentTimestamp();
        cs.topic = topic;
        cs.summary = summary;
        cs.turn_count = turns;
        cs.satisfaction_score = 0.7f;  // default
        profile.conversation_history.push_back(cs);
        profile.total_interactions += turns;
        save();
    }

    // Search facts by keyword
    std::vector<MKUserFact> searchFacts(const std::string& keyword) const {
        std::vector<MKUserFact> results;
        std::string lower_kw = keyword;
        std::transform(lower_kw.begin(), lower_kw.end(), lower_kw.begin(), ::tolower);
        for (const auto& fact : profile.facts) {
            std::string lower_key = fact.key, lower_val = fact.value;
            std::transform(lower_key.begin(), lower_key.end(), lower_key.begin(), ::tolower);
            std::transform(lower_val.begin(), lower_val.end(), lower_val.begin(), ::tolower);
            if (lower_key.find(lower_kw) != std::string::npos ||
                lower_val.find(lower_kw) != std::string::npos) {
                results.push_back(fact);
            }
        }
        return results;
    }

    // Save to disk
    void save() { saveToFile(); }

    // Get profile
    const MKUserProfile& getProfile() const { return profile; }

    void printStats() const {
        std::cout << "[MKMemoryManager] User: " << profile.user_name
                  << ", Facts: " << profile.facts.size()
                  << ", Topics: " << profile.topic_history.size()
                  << ", Interactions: " << profile.total_interactions << std::endl;
    }
};

#endif // MK_MEMORY_MANAGER_CPP
