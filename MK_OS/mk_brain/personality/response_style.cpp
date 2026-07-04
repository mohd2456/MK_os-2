#ifndef MK_RESPONSE_STYLE_CPP
#define MK_RESPONSE_STYLE_CPP

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <chrono>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <random>

// ===================================================================================
// MK RESPONSE STYLE
// ===================================================================================
// Makes MK's responses feel natural and contextually appropriate.
//
// Personality Modes:
//   - CONCISE:    Short, direct answers. Minimal elaboration.
//   - DETAILED:   Thorough explanations with context and examples.
//   - FRIENDLY:   Warm, conversational tone with encouragement.
//   - TECHNICAL:  Precise terminology, code-oriented, structured.
//   - TEACHER:    Step-by-step explanations, analogies, questions.
//
// Features:
//   - Adapts based on query type (technical -> TECHNICAL, casual -> FRIENDLY)
//   - Adds contextual confidence phrases
//   - Time-aware greetings
//   - Remembers user's preferred style
// ===================================================================================

enum class PersonalityMode {
    CONCISE,
    DETAILED,
    FRIENDLY,
    TECHNICAL,
    TEACHER
};

struct StylePreference {
    std::string user_id;
    PersonalityMode preferred_mode;
    int interactions_count;
    std::string last_interaction;
};

class MKResponseStyle {
private:
    PersonalityMode current_mode_;
    PersonalityMode default_mode_;
    std::map<std::string, StylePreference> user_preferences_;
    std::string preferences_file_;
    std::mt19937 rng_;

    // Contextual phrases organized by confidence level
    std::vector<std::string> high_confidence_phrases_;
    std::vector<std::string> medium_confidence_phrases_;
    std::vector<std::string> low_confidence_phrases_;
    std::vector<std::string> uncertain_phrases_;

    // Mode-specific response templates
    std::map<PersonalityMode, std::vector<std::string>> mode_prefixes_;
    std::map<PersonalityMode, std::vector<std::string>> mode_suffixes_;

    void init_phrases() {
        high_confidence_phrases_ = {
            ""
        };

        medium_confidence_phrases_ = {
            "From what I know, ",
            "Based on available info, ",
            ""
        };

        low_confidence_phrases_ = {
            "I'm not entirely sure, but ",
            "From limited data, "
        };

        uncertain_phrases_ = {
            "I'm not sure about this, but ",
            "I'd recommend verifying this elsewhere. My best guess: "
        };

        // Mode-specific prefixes
        mode_prefixes_[PersonalityMode::CONCISE] = {
            "", ""
        };
        mode_prefixes_[PersonalityMode::DETAILED] = {
            "Here's what I found: ",
            ""
        };
        mode_prefixes_[PersonalityMode::FRIENDLY] = {
            "",
            "",
            ""
        };
        mode_prefixes_[PersonalityMode::TECHNICAL] = {
            "",
            ""
        };
        mode_prefixes_[PersonalityMode::TEACHER] = {
            "Let me break this down: ",
            ""
        };

        // Mode-specific suffixes
        mode_suffixes_[PersonalityMode::CONCISE] = {
            "", "."
        };
        mode_suffixes_[PersonalityMode::DETAILED] = {
            " Let me know if you'd like more detail.",
            ""
        };
        mode_suffixes_[PersonalityMode::FRIENDLY] = {
            "",
            "",
            ""
        };
        mode_suffixes_[PersonalityMode::TECHNICAL] = {
            "",
            ""
        };
        mode_suffixes_[PersonalityMode::TEACHER] = {
            " Let me know if that makes sense.",
            ""
        };
    }

    std::string pick_random(const std::vector<std::string>& choices) {
        if (choices.empty()) return "";
        std::uniform_int_distribution<size_t> dist(0, choices.size() - 1);
        return choices[dist(rng_)];
    }

    int get_current_hour() {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        struct tm* tm_info = std::localtime(&time_t_now);
        return tm_info->tm_hour;
    }

    // Detect query type to auto-select mode
    PersonalityMode detect_query_type(const std::string& query) {
        std::string lower = query;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        // Technical indicators
        std::vector<std::string> tech_keywords = {
            "code", "function", "class", "api", "compile", "debug", "error",
            "implement", "algorithm", "syntax", "library", "framework",
            "server", "database", "deploy", "docker", "git", "binary"
        };

        // Teaching request indicators
        std::vector<std::string> teach_keywords = {
            "explain", "how does", "why does", "what is", "teach me",
            "help me understand", "walk me through", "tutorial", "learn"
        };

        // Casual/friendly indicators
        std::vector<std::string> casual_keywords = {
            "hey", "hi", "hello", "thanks", "cool", "awesome", "fun",
            "chat", "joke", "story", "favorite", "opinion"
        };

        // Detail request indicators
        std::vector<std::string> detail_keywords = {
            "in detail", "elaborate", "thoroughly", "comprehensive",
            "all about", "everything about", "deep dive"
        };

        int tech_score = 0, teach_score = 0, casual_score = 0, detail_score = 0;

        for (const auto& kw : tech_keywords) {
            if (lower.find(kw) != std::string::npos) tech_score++;
        }
        for (const auto& kw : teach_keywords) {
            if (lower.find(kw) != std::string::npos) teach_score++;
        }
        for (const auto& kw : casual_keywords) {
            if (lower.find(kw) != std::string::npos) casual_score++;
        }
        for (const auto& kw : detail_keywords) {
            if (lower.find(kw) != std::string::npos) detail_score++;
        }

        // Return highest scoring mode
        int max_score = std::max({tech_score, teach_score, casual_score, detail_score});
        if (max_score == 0) return current_mode_;

        if (tech_score == max_score) return PersonalityMode::TECHNICAL;
        if (teach_score == max_score) return PersonalityMode::TEACHER;
        if (detail_score == max_score) return PersonalityMode::DETAILED;
        if (casual_score == max_score) return PersonalityMode::FRIENDLY;

        return current_mode_;
    }

public:
    MKResponseStyle() : current_mode_(PersonalityMode::FRIENDLY),
                        default_mode_(PersonalityMode::FRIENDLY),
                        preferences_file_("mk_brain/personality/style_prefs.dat") {
        std::cout << "[RESPONSE_STYLE] Personality response style module loaded.\n";
        std::random_device rd;
        rng_ = std::mt19937(rd());
        init_phrases();
        load_preferences();
    }

    // ---------------------------------------------------------------
    // Time-Aware Greeting
    // ---------------------------------------------------------------
    std::string get_greeting(const std::string& user_id = "") {
        int hour = get_current_hour();
        std::string greeting;

        if (hour >= 5 && hour < 12) {
            greeting = "Good morning";
        } else if (hour >= 12 && hour < 17) {
            greeting = "Good afternoon";
        } else if (hour >= 17 && hour < 21) {
            greeting = "Good evening";
        } else {
            greeting = "Hey there, night owl";
        }

        // Personalize if we know the user
        if (!user_id.empty() && user_preferences_.count(user_id)) {
            auto& pref = user_preferences_[user_id];
            if (pref.interactions_count > 5) {
                greeting += "! Welcome back";
            }
        }

        return greeting + "! ";
    }

    // ---------------------------------------------------------------
    // Format Response with Style
    // ---------------------------------------------------------------
    std::string format_response(const std::string& raw_content, const std::string& query,
                                float confidence = 0.8f, const std::string& user_id = "") {
        // Auto-detect appropriate mode from query
        PersonalityMode mode = detect_query_type(query);

        // Override with user preference if known
        if (!user_id.empty() && user_preferences_.count(user_id)) {
            mode = user_preferences_[user_id].preferred_mode;
        }

        std::string formatted;

        // Add confidence phrase based on confidence level
        if (confidence >= 0.85f) {
            formatted += pick_random(high_confidence_phrases_);
        } else if (confidence >= 0.6f) {
            formatted += pick_random(medium_confidence_phrases_);
        } else if (confidence >= 0.3f) {
            formatted += pick_random(low_confidence_phrases_);
        } else {
            formatted += pick_random(uncertain_phrases_);
        }

        // Add mode-specific prefix (sometimes)
        std::uniform_int_distribution<int> coin(0, 2);
        if (coin(rng_) == 0) {
            formatted = pick_random(mode_prefixes_[mode]) + formatted;
        }

        // Add the core content
        formatted += raw_content;

        // Add mode-specific suffix
        formatted += pick_random(mode_suffixes_[mode]);

        // Track interaction
        if (!user_id.empty()) {
            if (user_preferences_.count(user_id) == 0) {
                user_preferences_[user_id] = {user_id, default_mode_, 0, ""};
            }
            user_preferences_[user_id].interactions_count++;
            auto now = std::chrono::system_clock::now();
            auto t = std::chrono::system_clock::to_time_t(now);
            user_preferences_[user_id].last_interaction = std::ctime(&t);
        }

        return formatted;
    }

    // ---------------------------------------------------------------
    // Mode Management
    // ---------------------------------------------------------------
    void set_mode(PersonalityMode mode) {
        current_mode_ = mode;
        std::cout << "[RESPONSE_STYLE] Mode set to: " << mode_name(mode) << "\n";
    }

    void set_user_preference(const std::string& user_id, PersonalityMode mode) {
        if (user_preferences_.count(user_id) == 0) {
            user_preferences_[user_id] = {user_id, mode, 0, ""};
        } else {
            user_preferences_[user_id].preferred_mode = mode;
        }
        save_preferences();
        std::cout << "[RESPONSE_STYLE] User " << user_id << " preference set to: " << mode_name(mode) << "\n";
    }

    PersonalityMode get_mode() const { return current_mode_; }

    std::string mode_name(PersonalityMode mode) {
        switch (mode) {
            case PersonalityMode::CONCISE:   return "CONCISE";
            case PersonalityMode::DETAILED:  return "DETAILED";
            case PersonalityMode::FRIENDLY:  return "FRIENDLY";
            case PersonalityMode::TECHNICAL: return "TECHNICAL";
            case PersonalityMode::TEACHER:   return "TEACHER";
        }
        return "UNKNOWN";
    }

    PersonalityMode mode_from_string(const std::string& name) {
        std::string upper = name;
        std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
        if (upper == "CONCISE") return PersonalityMode::CONCISE;
        if (upper == "DETAILED") return PersonalityMode::DETAILED;
        if (upper == "FRIENDLY") return PersonalityMode::FRIENDLY;
        if (upper == "TECHNICAL") return PersonalityMode::TECHNICAL;
        if (upper == "TEACHER") return PersonalityMode::TEACHER;
        return PersonalityMode::FRIENDLY; // default
    }

    // ---------------------------------------------------------------
    // Persistence
    // ---------------------------------------------------------------
    void save_preferences() {
        std::ofstream f(preferences_file_);
        if (!f.is_open()) return;
        for (const auto& [uid, pref] : user_preferences_) {
            f << uid << "|" << mode_name(pref.preferred_mode) << "|"
              << pref.interactions_count << "\n";
        }
        f.close();
    }

    void load_preferences() {
        std::ifstream f(preferences_file_);
        if (!f.is_open()) return;
        std::string line;
        while (std::getline(f, line)) {
            if (line.empty()) continue;
            size_t p1 = line.find('|');
            size_t p2 = line.find('|', p1 + 1);
            if (p1 == std::string::npos || p2 == std::string::npos) continue;

            std::string uid = line.substr(0, p1);
            std::string mode_str = line.substr(p1 + 1, p2 - p1 - 1);
            int count = std::stoi(line.substr(p2 + 1));

            user_preferences_[uid] = {uid, mode_from_string(mode_str), count, ""};
        }
        f.close();
        std::cout << "[RESPONSE_STYLE] Loaded " << user_preferences_.size() << " user preferences.\n";
    }
};

#endif // MK_RESPONSE_STYLE_CPP
