#ifndef MK_CONTEXT_TRACKER_CPP
#define MK_CONTEXT_TRACKER_CPP

#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <sstream>
#include <algorithm>

// ===========================================================================
// MK CONTEXT TRACKER - Multi-turn Conversation Context
// ===========================================================================
// Tracks conversation context across turns. Resolves pronouns,
// maintains topic stack, detects topic changes, handles multi-turn Q&A.
// Context window: last 30 turns.
// ===========================================================================

// A single conversation turn
struct MKConversationTurn {
    int turn_number;
    std::string speaker;     // 'user' or 'mk'
    std::string text;
    std::string topic;       // detected topic for this turn
    std::vector<std::string> entities;  // named entities mentioned
    std::string timestamp;
};

// Reference that can be resolved (pronoun -> entity)
struct MKReference {
    std::string pronoun;     // 'it', 'that', 'they', 'this'
    std::string resolved_to; // what it refers to
    int source_turn;         // which turn established the reference
    float confidence;
};

// Topic in the topic stack
struct MKTopic {
    std::string name;
    int started_at_turn;
    int last_active_turn;
    bool is_active;
};

class MKContextTracker {
private:
    static const int MAX_CONTEXT_WINDOW = 30;
    std::deque<MKConversationTurn> context_window;
    std::vector<MKTopic> topic_stack;
    std::vector<MKReference> active_references;
    std::unordered_map<std::string, std::string> entity_registry;
    int current_turn;
    std::string current_topic;

    // Extract entities from text (simple NER)
    std::vector<std::string> extractEntities(const std::string& text) {
        std::vector<std::string> entities;
        std::stringstream ss(text);
        std::string word;
        std::string prev_word;

        while (ss >> word) {
            // Capitalized words (not at start of sentence) are likely entities
            if (!prev_word.empty() && !word.empty() &&
                std::isupper(word[0]) && word.length() > 1) {
                entities.push_back(word);
            }
            // Technical terms in backticks or quotes
            if (word.front() == '`' || word.front() == '\'') {
                std::string entity = word.substr(1);
                if (!entity.empty() && (entity.back() == '`' || entity.back() == '\''))
                    entity.pop_back();
                if (!entity.empty()) entities.push_back(entity);
            }
            prev_word = word;
        }
        return entities;
    }

    // Detect the topic of a message
    std::string detectTopic(const std::string& text) {
        std::string lower = text;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        // Topic detection keywords
        std::vector<std::pair<std::string, std::vector<std::string>>> topic_keywords = {
            {"coding", {"code", "function", "class", "bug", "program", "compile", "debug"}},
            {"web", {"website", "html", "css", "react", "frontend", "backend", "api"}},
            {"data", {"database", "sql", "data", "table", "query", "schema"}},
            {"system", {"linux", "os", "kernel", "process", "memory", "disk"}},
            {"math", {"calculate", "equation", "formula", "number", "math"}},
            {"general", {"what", "how", "why", "explain", "tell me"}},
            {"project", {"project", "file", "directory", "structure", "build"}},
            {"help", {"help", "stuck", "error", "fix", "broken", "issue"}},
        };

        std::string best_topic = "general";
        int best_score = 0;
        for (const auto& [topic, keywords] : topic_keywords) {
            int score = 0;
            for (const auto& kw : keywords) {
                if (lower.find(kw) != std::string::npos) score++;
            }
            if (score > best_score) {
                best_score = score;
                best_topic = topic;
            }
        }
        return best_topic;
    }

    // Check if topic has changed significantly
    bool hasTopicChanged(const std::string& new_topic) {
        if (topic_stack.empty()) return true;
        return topic_stack.back().name != new_topic;
    }

    // Resolve pronouns in text using context
    std::string resolvePronouns(const std::string& text) {
        std::string resolved = text;
        std::vector<std::pair<std::string, std::string>> pronouns = {
            {" it ", ""}, {" that ", ""}, {" this ", ""},
            {" they ", ""}, {" them ", ""}, {" those ", ""}
        };

        // Find most recent entity to use as referent
        std::string last_entity;
        for (int i = static_cast<int>(context_window.size()) - 1; i >= 0; i--) {
            if (!context_window[i].entities.empty()) {
                last_entity = context_window[i].entities.back();
                break;
            }
        }

        if (last_entity.empty()) return text;

        // Update active references
        for (auto& [pronoun, ref] : pronouns) {
            if (text.find(pronoun) != std::string::npos) {
                active_references.push_back({pronoun, last_entity, current_turn, 0.7f});
            }
        }
        return resolved;  // Return original but track the reference
    }

public:
    MKContextTracker() : current_turn(0), current_topic("general") {}

    // Add a new turn to the context
    void addTurn(const std::string& speaker, const std::string& text) {
        current_turn++;

        MKConversationTurn turn;
        turn.turn_number = current_turn;
        turn.speaker = speaker;
        turn.text = text;
        turn.entities = extractEntities(text);
        turn.topic = detectTopic(text);

        // Resolve pronouns
        resolvePronouns(text);

        // Register entities
        for (const auto& entity : turn.entities) {
            entity_registry[entity] = text;
        }

        // Update topic stack
        if (hasTopicChanged(turn.topic)) {
            // Mark old topic as inactive
            if (!topic_stack.empty()) {
                topic_stack.back().is_active = false;
            }
            topic_stack.push_back({turn.topic, current_turn, current_turn, true});
            current_topic = turn.topic;
        } else if (!topic_stack.empty()) {
            topic_stack.back().last_active_turn = current_turn;
        }

        // Add to context window (maintain max size)
        context_window.push_back(turn);
        if (static_cast<int>(context_window.size()) > MAX_CONTEXT_WINDOW) {
            context_window.pop_front();
        }
    }

    // Get the current topic
    std::string getCurrentTopic() const { return current_topic; }

    // Get last N turns as context string
    std::string getRecentContext(int n = 5) const {
        std::string context;
        int start = std::max(0, static_cast<int>(context_window.size()) - n);
        for (int i = start; i < static_cast<int>(context_window.size()); i++) {
            const auto& turn = context_window[i];
            context += "[" + turn.speaker + "] " + turn.text + "\n";
        }
        return context;
    }

    // Resolve a pronoun reference
    std::string resolveReference(const std::string& pronoun) const {
        for (int i = static_cast<int>(active_references.size()) - 1; i >= 0; i--) {
            if (active_references[i].pronoun.find(pronoun) != std::string::npos) {
                return active_references[i].resolved_to;
            }
        }
        return "";
    }

    // Get all mentioned entities
    std::vector<std::string> getEntities() const {
        std::vector<std::string> entities;
        for (const auto& [entity, context] : entity_registry) {
            entities.push_back(entity);
        }
        return entities;
    }

    // Get the topic history
    std::vector<std::string> getTopicHistory() const {
        std::vector<std::string> topics;
        for (const auto& t : topic_stack) {
            topics.push_back(t.name);
        }
        return topics;
    }

    // Check if a specific topic was discussed recently
    bool wasTopicDiscussed(const std::string& topic) const {
        for (const auto& t : topic_stack) {
            if (t.name == topic) return true;
        }
        return false;
    }

    // Get conversation length
    int getTurnCount() const { return current_turn; }
    int getContextSize() const { return static_cast<int>(context_window.size()); }

    // Reset context (new conversation)
    void reset() {
        context_window.clear();
        topic_stack.clear();
        active_references.clear();
        entity_registry.clear();
        current_turn = 0;
        current_topic = "general";
    }

    void printStats() const {
        std::cout << "[MKContextTracker] Turns: " << current_turn
                  << ", Topic: " << current_topic
                  << ", Entities: " << entity_registry.size() << std::endl;
    }
};

#endif // MK_CONTEXT_TRACKER_CPP
