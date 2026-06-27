#ifndef MK_DIALOGUE_MANAGER_CPP
#define MK_DIALOGUE_MANAGER_CPP

#include <string>
#include <vector>
#include <deque>
#include <map>
#include <set>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <chrono>
#include <ctime>

// ============================================================================
// MKDialogueManager - Multi-turn conversation management
// Tracks context, detects topic changes, and manages conversation state
// ============================================================================

enum class ConversationState {
    GREETING,
    QUESTION,
    FOLLOWUP,
    DISCUSSION,
    COMMAND,
    GOODBYE,
    IDLE
};

enum class MessageRole {
    USER,
    ASSISTANT,
    SYSTEM
};

enum class IntentType {
    ASK_QUESTION,
    GIVE_COMMAND,
    MAKE_STATEMENT,
    EXPRESS_EMOTION,
    REQUEST_CLARIFICATION,
    CHANGE_TOPIC,
    END_CONVERSATION
};

struct Message {
    MessageRole role;
    std::string text;
    std::string timestamp;
    std::string detected_topic;
    IntentType intent;
    int turn_number;
};

struct TopicSegment {
    std::string topic;
    int start_turn;
    int end_turn;
    int message_count;
};

class MKDialogueManager {
private:
    std::deque<Message> context_window_;
    int max_context_size_;
    int current_turn_;
    ConversationState current_state_;
    std::string current_topic_;
    std::vector<TopicSegment> topic_history_;
    std::map<std::string, int> topic_keywords_;
    std::set<std::string> greeting_words_;
    std::set<std::string> goodbye_words_;
    std::set<std::string> question_words_;

    void initializeKeywords() {
        greeting_words_ = {"hello", "hi", "hey", "greetings", "good morning",
                          "good afternoon", "good evening", "howdy", "sup", "yo"};
        goodbye_words_ = {"bye", "goodbye", "see you", "later", "quit", "exit",
                         "cya", "farewell", "goodnight", "take care"};
        question_words_ = {"what", "how", "why", "when", "where", "who", "which",
                          "can", "could", "would", "should", "is", "are", "do", "does"};
    }

    std::string getCurrentTimestamp() const {
        auto now = std::chrono::system_clock::now();
        time_t time = std::chrono::system_clock::to_time_t(now);
        std::string ts = std::ctime(&time);
        if (!ts.empty() && ts.back() == '\n') ts.pop_back();
        return ts;
    }

    std::string toLower(const std::string& str) const {
        std::string lower = str;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        return lower;
    }

    std::vector<std::string> tokenize(const std::string& text) const {
        std::vector<std::string> words;
        std::istringstream stream(toLower(text));
        std::string word;
        while (stream >> word) {
            // Remove punctuation
            word.erase(std::remove_if(word.begin(), word.end(), ::ispunct), word.end());
            if (!word.empty()) words.push_back(word);
        }
        return words;
    }

    bool containsAny(const std::string& text, const std::set<std::string>& words) const {
        std::string lower = toLower(text);
        for (const auto& w : words) {
            if (lower.find(w) != std::string::npos) return true;
        }
        return false;
    }

    IntentType classifyIntent(const std::string& text) const {
        std::string lower = toLower(text);

        if (containsAny(text, greeting_words_) && text.length() < 30) {
            return IntentType::ASK_QUESTION; // will be overridden by state
        }
        if (containsAny(text, goodbye_words_)) {
            return IntentType::END_CONVERSATION;
        }

        // Check if question
        if (lower.back() == '?' || lower.find("?") != std::string::npos) {
            return IntentType::ASK_QUESTION;
        }
        std::vector<std::string> words = tokenize(text);
        if (!words.empty() && question_words_.count(words[0])) {
            return IntentType::ASK_QUESTION;
        }

        // Check if command
        std::vector<std::string> command_starts = {"run", "do", "make", "create",
            "delete", "open", "close", "show", "find", "search", "write", "build"};
        if (!words.empty()) {
            for (const auto& cmd : command_starts) {
                if (words[0] == cmd) return IntentType::GIVE_COMMAND;
            }
        }

        return IntentType::MAKE_STATEMENT;
    }

    std::string extractTopic(const std::string& text) const {
        std::vector<std::string> words = tokenize(text);
        // Simple topic extraction: find nouns (longer words that aren't common)
        std::set<std::string> stopwords = {"the", "a", "an", "is", "are", "was", "were",
            "be", "been", "being", "have", "has", "had", "do", "does", "did", "will",
            "would", "could", "should", "may", "might", "shall", "can", "need", "dare",
            "to", "of", "in", "for", "on", "with", "at", "by", "from", "it", "this",
            "that", "and", "or", "but", "if", "not", "no", "so", "i", "you", "me", "my"};

        std::string topic;
        for (const auto& word : words) {
            if (word.length() > 3 && !stopwords.count(word)) {
                if (!topic.empty()) topic += " ";
                topic += word;
                if (topic.length() > 30) break;
            }
        }
        return topic.empty() ? "general" : topic;
    }

    bool isTopicChange(const std::string& new_topic) const {
        if (current_topic_.empty()) return false;
        if (new_topic == current_topic_) return false;

        // Check word overlap
        std::vector<std::string> curr_words = tokenize(current_topic_);
        std::vector<std::string> new_words = tokenize(new_topic);

        int overlap = 0;
        for (const auto& w : new_words) {
            for (const auto& cw : curr_words) {
                if (w == cw) { overlap++; break; }
            }
        }

        float overlap_ratio = static_cast<float>(overlap) / 
                             static_cast<float>(std::max(curr_words.size(), new_words.size()));
        return overlap_ratio < 0.3f;
    }

    void updateState(const Message& msg) {
        if (msg.role != MessageRole::USER) return;

        std::string lower = toLower(msg.text);

        if (current_turn_ <= 2 && containsAny(msg.text, greeting_words_)) {
            current_state_ = ConversationState::GREETING;
        } else if (containsAny(msg.text, goodbye_words_)) {
            current_state_ = ConversationState::GOODBYE;
        } else if (msg.intent == IntentType::ASK_QUESTION) {
            if (current_state_ == ConversationState::QUESTION) {
                current_state_ = ConversationState::FOLLOWUP;
            } else {
                current_state_ = ConversationState::QUESTION;
            }
        } else if (msg.intent == IntentType::GIVE_COMMAND) {
            current_state_ = ConversationState::COMMAND;
        } else {
            current_state_ = ConversationState::DISCUSSION;
        }
    }

public:
    MKDialogueManager() : max_context_size_(50), current_turn_(0),
                          current_state_(ConversationState::IDLE) {
        initializeKeywords();
    }

    explicit MKDialogueManager(int context_size) 
        : max_context_size_(context_size), current_turn_(0),
          current_state_(ConversationState::IDLE) {
        initializeKeywords();
    }

    void addMessage(MessageRole role, const std::string& text) {
        current_turn_++;
        Message msg;
        msg.role = role;
        msg.text = text;
        msg.timestamp = getCurrentTimestamp();
        msg.turn_number = current_turn_;
        msg.intent = classifyIntent(text);
        msg.detected_topic = extractTopic(text);

        // Check for topic change
        if (role == MessageRole::USER && isTopicChange(msg.detected_topic)) {
            if (!current_topic_.empty()) {
                TopicSegment seg;
                seg.topic = current_topic_;
                seg.end_turn = current_turn_ - 1;
                if (!topic_history_.empty()) {
                    seg.start_turn = topic_history_.back().end_turn + 1;
                } else {
                    seg.start_turn = 1;
                }
                seg.message_count = seg.end_turn - seg.start_turn + 1;
                topic_history_.push_back(seg);
            }
            current_topic_ = msg.detected_topic;
        } else if (current_topic_.empty()) {
            current_topic_ = msg.detected_topic;
        }

        updateState(msg);

        // Add to context window
        context_window_.push_back(msg);
        if (static_cast<int>(context_window_.size()) > max_context_size_) {
            context_window_.pop_front();
        }
    }

    std::vector<Message> getContext() const {
        return std::vector<Message>(context_window_.begin(), context_window_.end());
    }

    IntentType detectIntent() const {
        if (context_window_.empty()) return IntentType::MAKE_STATEMENT;
        return context_window_.back().intent;
    }

    std::vector<TopicSegment> getTopicHistory() const {
        return topic_history_;
    }

    ConversationState getState() const { return current_state_; }
    std::string getCurrentTopic() const { return current_topic_; }
    int getTurnCount() const { return current_turn_; }
    int getContextSize() const { return static_cast<int>(context_window_.size()); }

    std::string getStateName() const {
        switch (current_state_) {
            case ConversationState::GREETING: return "greeting";
            case ConversationState::QUESTION: return "question";
            case ConversationState::FOLLOWUP: return "followup";
            case ConversationState::DISCUSSION: return "discussion";
            case ConversationState::COMMAND: return "command";
            case ConversationState::GOODBYE: return "goodbye";
            case ConversationState::IDLE: return "idle";
            default: return "unknown";
        }
    }

    void reset() {
        context_window_.clear();
        current_turn_ = 0;
        current_state_ = ConversationState::IDLE;
        current_topic_.clear();
        topic_history_.clear();
    }
};

#endif // MK_DIALOGUE_MANAGER_CPP
