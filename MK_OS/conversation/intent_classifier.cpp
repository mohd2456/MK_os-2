#ifndef MK_INTENT_CLASSIFIER_CPP
#define MK_INTENT_CLASSIFIER_CPP

#include <string>
#include <vector>
#include <map>
#include <set>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <cmath>

// ============================================================================
// MKIntentClassifier - Routes user input to the correct handler
// Uses keyword scoring + sentence structure analysis for classification
// ============================================================================

enum class Intent {
    QUESTION,
    COMMAND,
    CHAT,
    CODE_REQUEST,
    TEACHING,
    REMINDER,
    SEARCH,
    UNKNOWN
};

struct Entity {
    std::string type;       // e.g., "language", "topic", "time", "url"
    std::string value;
    int start_pos;
    int end_pos;
};

struct ClassificationResult {
    Intent intent;
    float confidence;
    std::vector<Entity> entities;
    std::string raw_input;
    std::map<Intent, float> all_scores;
};

class MKIntentClassifier {
private:
    std::map<Intent, std::vector<std::string>> intent_keywords_;
    std::map<Intent, std::vector<std::string>> intent_patterns_;
    std::map<Intent, float> intent_weights_;
    ClassificationResult last_result_;

    void initializeKeywords() {
        intent_keywords_[Intent::QUESTION] = {
            "what", "how", "why", "when", "where", "who", "which",
            "explain", "tell me", "describe", "meaning", "define",
            "difference", "compare", "is it", "can you tell"
        };
        intent_keywords_[Intent::COMMAND] = {
            "run", "execute", "do", "make", "create", "delete", "remove",
            "open", "close", "start", "stop", "install", "update",
            "build", "compile", "deploy", "restart", "kill", "launch"
        };
        intent_keywords_[Intent::CHAT] = {
            "hello", "hi", "hey", "thanks", "thank you", "cool", "nice",
            "awesome", "great", "ok", "okay", "sure", "wow", "lol",
            "haha", "interesting", "yeah", "yes", "no", "bye"
        };
        intent_keywords_[Intent::CODE_REQUEST] = {
            "write code", "write a", "code", "program", "script", "function",
            "implement", "algorithm", "class", "snippet", "example",
            "python", "javascript", "c++", "html", "css", "sql",
            "debug", "fix", "refactor", "optimize"
        };
        intent_keywords_[Intent::TEACHING] = {
            "teach", "learn", "tutorial", "explain how", "show me how",
            "walk me through", "step by step", "beginner", "understand",
            "concept", "basics", "introduction", "guide"
        };
        intent_keywords_[Intent::REMINDER] = {
            "remind", "remember", "don't forget", "note", "save",
            "later", "tomorrow", "schedule", "alarm", "timer",
            "at", "in 5 minutes", "next week"
        };
        intent_keywords_[Intent::SEARCH] = {
            "search", "find", "look up", "google", "browse",
            "website", "url", "link", "online", "internet",
            "latest", "news", "current", "recent"
        };
    }

    void initializeWeights() {
        intent_weights_[Intent::QUESTION] = 1.0f;
        intent_weights_[Intent::COMMAND] = 1.2f;
        intent_weights_[Intent::CHAT] = 0.8f;
        intent_weights_[Intent::CODE_REQUEST] = 1.3f;
        intent_weights_[Intent::TEACHING] = 1.1f;
        intent_weights_[Intent::REMINDER] = 1.2f;
        intent_weights_[Intent::SEARCH] = 1.1f;
    }

    std::string toLower(const std::string& str) const {
        std::string lower = str;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        return lower;
    }

    std::vector<std::string> tokenize(const std::string& text) const {
        std::vector<std::string> tokens;
        std::istringstream stream(toLower(text));
        std::string word;
        while (stream >> word) {
            word.erase(std::remove_if(word.begin(), word.end(), ::ispunct), word.end());
            if (!word.empty()) tokens.push_back(word);
        }
        return tokens;
    }

    float scoreKeywords(const std::string& input, Intent intent) const {
        std::string lower = toLower(input);
        float score = 0.0f;
        int matches = 0;

        auto it = intent_keywords_.find(intent);
        if (it == intent_keywords_.end()) return 0.0f;

        for (const auto& keyword : it->second) {
            if (lower.find(keyword) != std::string::npos) {
                matches++;
                // Bonus for keyword at start of input
                if (lower.find(keyword) == 0) {
                    score += 2.0f;
                } else {
                    score += 1.0f;
                }
            }
        }

        return score;
    }

    float scoreStructure(const std::string& input, Intent intent) const {
        std::string lower = toLower(input);
        float score = 0.0f;

        switch (intent) {
            case Intent::QUESTION:
                if (lower.back() == '?') score += 3.0f;
                if (lower.length() > 10 && lower.length() < 200) score += 0.5f;
                break;
            case Intent::COMMAND:
                if (lower.length() < 50) score += 1.0f;
                if (lower.find("please") != std::string::npos) score += 0.5f;
                // Imperative mood - starts with verb
                break;
            case Intent::CHAT:
                if (lower.length() < 20) score += 2.0f;
                if (lower.find('!') != std::string::npos) score += 0.5f;
                break;
            case Intent::CODE_REQUEST:
                if (lower.find("in python") != std::string::npos ||
                    lower.find("in c++") != std::string::npos ||
                    lower.find("in javascript") != std::string::npos) score += 3.0f;
                if (lower.find("that") != std::string::npos) score += 0.5f;
                break;
            case Intent::TEACHING:
                if (lower.find("how to") != std::string::npos) score += 2.0f;
                if (lower.find("step") != std::string::npos) score += 1.0f;
                break;
            case Intent::REMINDER:
                if (lower.find("at ") != std::string::npos) score += 1.5f;
                if (lower.find("tomorrow") != std::string::npos) score += 2.0f;
                break;
            case Intent::SEARCH:
                if (lower.find("what is") != std::string::npos) score += 1.0f;
                if (lower.find("latest") != std::string::npos) score += 1.5f;
                break;
            default:
                break;
        }
        return score;
    }

    std::vector<Entity> extractEntities(const std::string& input) const {
        std::vector<Entity> entities;
        std::string lower = toLower(input);

        // Extract programming languages
        std::vector<std::string> languages = {"python", "javascript", "c++", "java",
            "ruby", "go", "rust", "typescript", "html", "css", "sql"};
        for (const auto& lang : languages) {
            size_t pos = lower.find(lang);
            if (pos != std::string::npos) {
                Entity e;
                e.type = "language";
                e.value = lang;
                e.start_pos = static_cast<int>(pos);
                e.end_pos = static_cast<int>(pos + lang.length());
                entities.push_back(e);
            }
        }

        // Extract URLs
        std::vector<std::string> url_prefixes = {"http://", "https://", "www."};
        for (const auto& prefix : url_prefixes) {
            size_t pos = lower.find(prefix);
            if (pos != std::string::npos) {
                size_t end = lower.find_first_of(" \t\n", pos);
                if (end == std::string::npos) end = lower.length();
                Entity e;
                e.type = "url";
                e.value = input.substr(pos, end - pos);
                e.start_pos = static_cast<int>(pos);
                e.end_pos = static_cast<int>(end);
                entities.push_back(e);
            }
        }

        // Extract time references
        std::vector<std::string> time_words = {"tomorrow", "today", "tonight",
            "next week", "in an hour", "at noon", "this evening"};
        for (const auto& tw : time_words) {
            size_t pos = lower.find(tw);
            if (pos != std::string::npos) {
                Entity e;
                e.type = "time";
                e.value = tw;
                e.start_pos = static_cast<int>(pos);
                e.end_pos = static_cast<int>(pos + tw.length());
                entities.push_back(e);
            }
        }

        // Extract quoted strings as topics
        size_t quote_start = input.find('"');
        while (quote_start != std::string::npos) {
            size_t quote_end = input.find('"', quote_start + 1);
            if (quote_end != std::string::npos) {
                Entity e;
                e.type = "topic";
                e.value = input.substr(quote_start + 1, quote_end - quote_start - 1);
                e.start_pos = static_cast<int>(quote_start);
                e.end_pos = static_cast<int>(quote_end + 1);
                entities.push_back(e);
                quote_start = input.find('"', quote_end + 1);
            } else {
                break;
            }
        }

        return entities;
    }

public:
    MKIntentClassifier() {
        initializeKeywords();
        initializeWeights();
    }

    ClassificationResult classify(const std::string& input) {
        ClassificationResult result;
        result.raw_input = input;
        result.entities = extractEntities(input);

        float best_score = 0.0f;
        Intent best_intent = Intent::UNKNOWN;

        std::vector<Intent> all_intents = {
            Intent::QUESTION, Intent::COMMAND, Intent::CHAT,
            Intent::CODE_REQUEST, Intent::TEACHING, Intent::REMINDER, Intent::SEARCH
        };

        for (Intent intent : all_intents) {
            float keyword_score = scoreKeywords(input, intent);
            float structure_score = scoreStructure(input, intent);
            float weight = intent_weights_[intent];
            float total = (keyword_score + structure_score) * weight;

            result.all_scores[intent] = total;

            if (total > best_score) {
                best_score = total;
                best_intent = intent;
            }
        }

        result.intent = best_intent;

        // Calculate confidence (normalized)
        float sum = 0.0f;
        for (const auto& kv : result.all_scores) sum += kv.second;
        result.confidence = (sum > 0) ? (best_score / sum) : 0.0f;
        result.confidence = std::min(1.0f, std::max(0.0f, result.confidence));

        last_result_ = result;
        return result;
    }

    std::vector<Entity> getEntities() const { return last_result_.entities; }
    float getConfidence() const { return last_result_.confidence; }
    Intent getLastIntent() const { return last_result_.intent; }

    std::string intentToString(Intent intent) const {
        switch (intent) {
            case Intent::QUESTION: return "QUESTION";
            case Intent::COMMAND: return "COMMAND";
            case Intent::CHAT: return "CHAT";
            case Intent::CODE_REQUEST: return "CODE_REQUEST";
            case Intent::TEACHING: return "TEACHING";
            case Intent::REMINDER: return "REMINDER";
            case Intent::SEARCH: return "SEARCH";
            default: return "UNKNOWN";
        }
    }

    void addCustomKeyword(Intent intent, const std::string& keyword) {
        intent_keywords_[intent].push_back(keyword);
    }

    void setWeight(Intent intent, float weight) {
        intent_weights_[intent] = weight;
    }
};

#endif // MK_INTENT_CLASSIFIER_CPP
