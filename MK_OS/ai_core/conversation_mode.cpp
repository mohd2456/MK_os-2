#ifndef MK_CONVERSATION_MODE_CPP
#define MK_CONVERSATION_MODE_CPP

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>
#include <deque>

// ===================================================================================
// MK CONVERSATION MODE — "Friend Mode"
// ===================================================================================
// Handles casual conversation: greetings, venting, stories, emotional responses.
// All rule-based, no LLM required, runs on the weakest hardware.
// Makes MK feel like a friend, not a search engine.
//
// Features:
//   - Emotion detection from keywords
//   - Conversation vs question classification
//   - Story/vent detection for long inputs
//   - Follow-up memory (last 3 topics)
//   - Opinion generation from knowledge graph data
//   - Casual Gen-Z response templates
// ===================================================================================

// Detected mood types
enum class MKMood {
    HAPPY,
    SAD,
    ANGRY,
    NERVOUS,
    CHILL,
    NEUTRAL
};

// Result of emotion detection
struct MKMoodResult {
    MKMood mood;
    float confidence;  // 0.0 - 1.0
    std::string trigger_word;  // the keyword that triggered detection
};

// Conversation classification
enum class MKInputType {
    GREETING,
    GOODBYE,
    STORY,
    VENT,
    OPINION_SHARE,
    EMOTIONAL,
    SMALL_TALK,
    VAGUE_RESPONSE,    // "yeah", "true", "exactly"
    TOPIC_CHANGE,      // "anyway", "so"
    QUESTION,          // Not conversation — route normally
    COMMAND            // Not conversation — route normally
};

class MKConversationMode {
private:
    std::mt19937 rng_;
    
    // Follow-up memory: last 3 topics discussed
    std::deque<std::string> recentTopics_;
    static const size_t MAX_TOPICS = 3;

    // Keyword lists for mood detection
    std::vector<std::string> happy_keywords_ = {
        "hyped", "excited", "happy", "amazing", "great", "awesome",
        "lol", "lmao", "lets go", "let's go", "fire", "sick",
        "incredible", "perfect", "blessed", "winning", "W"
    };

    std::vector<std::string> sad_keywords_ = {
        "sad", "depressed", "tired", "exhausted", "failed", "sucks",
        "hate", "crying", "miss", "lonely", "hopeless", "drained",
        "hurts", "broken", "lost", "numb"
    };

    std::vector<std::string> angry_keywords_ = {
        "angry", "pissed", "annoyed", "frustrated", "wtf", "bs",
        "furious", "mad", "rage", "livid", "fuming", "sick of",
        "fed up", "done with", "can't stand"
    };

    std::vector<std::string> nervous_keywords_ = {
        "nervous", "scared", "worried", "anxious", "stressed",
        "freaking out", "panicking", "overthinking", "terrified",
        "dreading", "shaking", "can't sleep", "restless"
    };

    std::vector<std::string> chill_keywords_ = {
        "bored", "whatever", "idk", "meh", "chillin", "vibing",
        "just here", "nothing much", "same old", "eh", "kinda"
    };

    // Greeting triggers
    std::vector<std::string> greeting_triggers_ = {
        "yo", "hey", "sup", "what's good", "what's up", "hi",
        "hello", "heyo", "heyy", "yoo", "ayy", "ayo", "waddup",
        "howdy", "greetings", "hola", "whats good", "whats up"
    };

    // Goodbye triggers
    std::vector<std::string> goodbye_triggers_ = {
        "bye", "gotta go", "peace", "later", "see ya", "goodnight",
        "gn", "I'm out", "im out", "heading out", "ttyl", "cya",
        "deuces", "peace out", "bouncing", "signing off"
    };

    // Story indicators — words that signal narrative input
    std::vector<std::string> story_indicators_ = {
        "so today", "bro guess what", "man this thing happened",
        "you won't believe", "dude so", "ok so basically",
        "so basically", "long story short", "get this",
        "no but listen", "I swear", "not even kidding"
    };

    // Narrative flow words (for detecting stories in longer input)
    std::vector<std::string> narrative_words_ = {
        "so", "then", "and then", "basically", "like",
        "anyway", "right", "literally", "next thing",
        "after that", "turns out", "ended up"
    };

    // Opinion share indicators
    std::vector<std::string> opinion_indicators_ = {
        "I think", "I feel like", "honestly", "in my opinion",
        "imo", "ngl", "tbh", "I believe", "if you ask me",
        "personally", "to me", "I reckon"
    };

    // Vague response words (continues last topic)
    std::vector<std::string> vague_responses_ = {
        "yeah", "true", "exactly", "fr", "facts", "real",
        "right", "yep", "mhm", "ikr", "same", "literally",
        "lol", "lmao", "haha", "bruh", "ong", "no cap"
    };

    // Topic change indicators
    std::vector<std::string> topic_change_words_ = {
        "anyway", "so", "btw", "by the way", "random but",
        "off topic but", "unrelated but", "oh also", "switching gears"
    };

    // Question words
    std::vector<std::string> question_words_ = {
        "what", "who", "where", "when", "why", "how",
        "which", "is it", "are there", "can you", "do you",
        "does", "did", "will", "would", "could", "should"
    };

    // ---------------------------------------------------------------
    // Helper: convert string to lowercase
    // ---------------------------------------------------------------
    std::string toLower(const std::string& s) const {
        std::string result = s;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

    // ---------------------------------------------------------------
    // Helper: check if input contains any keyword from a list
    // ---------------------------------------------------------------
    bool containsAny(const std::string& lower, const std::vector<std::string>& keywords) const {
        for (const auto& kw : keywords) {
            if (lower.find(kw) != std::string::npos) {
                return true;
            }
        }
        return false;
    }

    // ---------------------------------------------------------------
    // Helper: find which keyword matched
    // ---------------------------------------------------------------
    std::string findMatch(const std::string& lower, const std::vector<std::string>& keywords) const {
        for (const auto& kw : keywords) {
            if (lower.find(kw) != std::string::npos) {
                return kw;
            }
        }
        return "";
    }

    // ---------------------------------------------------------------
    // Helper: count words in input
    // ---------------------------------------------------------------
    int wordCount(const std::string& input) const {
        int count = 0;
        bool inWord = false;
        for (char c : input) {
            if (c == ' ' || c == '\t' || c == '\n') {
                inWord = false;
            } else if (!inWord) {
                inWord = true;
                count++;
            }
        }
        return count;
    }

    // ---------------------------------------------------------------
    // Helper: count how many keywords from a list appear in input
    // ---------------------------------------------------------------
    int countMatches(const std::string& lower, const std::vector<std::string>& keywords) const {
        int count = 0;
        for (const auto& kw : keywords) {
            if (lower.find(kw) != std::string::npos) {
                count++;
            }
        }
        return count;
    }

    // ---------------------------------------------------------------
    // Helper: check if input starts with any prefix from list
    // ---------------------------------------------------------------
    bool startsWithAny(const std::string& lower, const std::vector<std::string>& prefixes) const {
        for (const auto& prefix : prefixes) {
            if (lower.length() >= prefix.length() &&
                lower.substr(0, prefix.length()) == prefix) {
                return true;
            }
        }
        return false;
    }

    // ---------------------------------------------------------------
    // Helper: check if input is just a greeting (short, matches trigger)
    // ---------------------------------------------------------------
    bool isGreeting(const std::string& lower) const {
        // Greetings are typically short (1-4 words)
        if (wordCount(lower) > 5) return false;
        
        for (const auto& g : greeting_triggers_) {
            if (lower == g || lower.find(g) == 0) {
                return true;
            }
        }
        return false;
    }

    // ---------------------------------------------------------------
    // Helper: check if input is a goodbye
    // ---------------------------------------------------------------
    bool isGoodbye(const std::string& lower) const {
        if (wordCount(lower) > 6) return false;
        return containsAny(lower, goodbye_triggers_);
    }

public:
    // ---------------------------------------------------------------
    // Constructor
    // ---------------------------------------------------------------
    MKConversationMode() {
        auto seed = std::chrono::steady_clock::now().time_since_epoch().count();
        rng_ = std::mt19937(static_cast<unsigned>(seed));
        std::cout << "[CONVERSATION_MODE] Friend mode engine initialized.\n";
    }

    // ---------------------------------------------------------------
    // EMOTION DETECTION
    // Returns detected mood + confidence from keyword analysis
    // ---------------------------------------------------------------
    MKMoodResult detectMood(const std::string& input) {
        std::string lower = toLower(input);
        MKMoodResult result;
        result.mood = MKMood::NEUTRAL;
        result.confidence = 0.0f;
        result.trigger_word = "";

        // Count matches in each mood category
        int happy_score = countMatches(lower, happy_keywords_);
        int sad_score = countMatches(lower, sad_keywords_);
        int angry_score = countMatches(lower, angry_keywords_);
        int nervous_score = countMatches(lower, nervous_keywords_);
        int chill_score = countMatches(lower, chill_keywords_);

        // Find the highest scoring mood
        int max_score = std::max({happy_score, sad_score, angry_score, nervous_score, chill_score});
        
        if (max_score == 0) {
            result.mood = MKMood::NEUTRAL;
            result.confidence = 0.0f;
            return result;
        }

        // Assign mood based on highest score
        if (happy_score == max_score) {
            result.mood = MKMood::HAPPY;
            result.trigger_word = findMatch(lower, happy_keywords_);
        } else if (sad_score == max_score) {
            result.mood = MKMood::SAD;
            result.trigger_word = findMatch(lower, sad_keywords_);
        } else if (angry_score == max_score) {
            result.mood = MKMood::ANGRY;
            result.trigger_word = findMatch(lower, angry_keywords_);
        } else if (nervous_score == max_score) {
            result.mood = MKMood::NERVOUS;
            result.trigger_word = findMatch(lower, nervous_keywords_);
        } else if (chill_score == max_score) {
            result.mood = MKMood::CHILL;
            result.trigger_word = findMatch(lower, chill_keywords_);
        }

        // Confidence based on keyword density
        int total_words = wordCount(input);
        if (total_words > 0) {
            result.confidence = std::min(1.0f, (float)max_score / std::max(1, total_words / 3));
        } else {
            result.confidence = 0.5f;
        }

        // Boost confidence if multiple keywords from same category
        if (max_score >= 3) result.confidence = std::min(1.0f, result.confidence + 0.2f);
        if (max_score >= 2) result.confidence = std::min(1.0f, result.confidence + 0.1f);

        return result;
    }

    // ---------------------------------------------------------------
    // CONVERSATION vs QUESTION DETECTION
    // Returns true if input is conversational (not a question/command)
    // ---------------------------------------------------------------
    bool isConversation(const std::string& input) {
        std::string lower = toLower(input);
        int words = wordCount(input);

        // Empty or single character — not a conversation
        if (words == 0) return false;

        // Slash commands are never conversation
        if (!input.empty() && input[0] == '/') return false;

        // Greetings are conversation
        if (isGreeting(lower)) return true;

        // Goodbyes are conversation
        if (isGoodbye(lower)) return true;

        // Vague responses ("yeah", "true", "fr") are conversation
        if (words <= 3) {
            for (const auto& vr : vague_responses_) {
                if (lower == vr || lower == vr + "." || lower == vr + "!") {
                    return true;
                }
            }
        }

        // Topic change phrases
        if (startsWithAny(lower, topic_change_words_)) return true;

        // Has a question mark — likely a question (unless "fr?" or "right?" type)
        if (input.find('?') != std::string::npos) {
            // Short reactive questions are still conversation
            if (words <= 3) return true;  // "fr?", "you good?", "really?"
            
            // "what do you think about X" is conversation (opinion request)
            if (lower.find("what do you think") != std::string::npos) return true;
            if (lower.find("what you think") != std::string::npos) return true;
            
            // Otherwise it's a question — route normally
            return false;
        }

        // Starts with question words — likely a question
        for (const auto& qw : question_words_) {
            if (lower.length() >= qw.length() + 1 &&
                lower.substr(0, qw.length()) == qw &&
                (lower[qw.length()] == ' ' || lower[qw.length()] == '\t')) {
                // Exception: "how are you" type queries are conversation
                if (lower.find("how are you") != std::string::npos) return true;
                if (lower.find("how you doing") != std::string::npos) return true;
                return false;
            }
        }

        // Story indicators
        if (containsAny(lower, story_indicators_)) return true;

        // Opinion sharing
        if (startsWithAny(lower, opinion_indicators_)) return true;

        // Emotional expression (strong mood detected)
        MKMoodResult mood = detectMood(input);
        if (mood.confidence >= 0.4f) return true;

        // Long input with narrative words = story/vent
        if (words > 15 && countMatches(lower, narrative_words_) >= 2) return true;

        // Short statements without question structure — conversation
        if (words <= 6 && input.find('?') == std::string::npos) {
            // Check it's not a clear command-like phrase
            if (lower.find("search") == std::string::npos &&
                lower.find("find") == std::string::npos &&
                lower.find("tell me") == std::string::npos &&
                lower.find("show me") == std::string::npos &&
                lower.find("look up") == std::string::npos) {
                return true;
            }
        }

        return false;
    }

    // ---------------------------------------------------------------
    // CLASSIFY INPUT TYPE
    // More specific than just isConversation — returns what kind
    // ---------------------------------------------------------------
    MKInputType classifyInput(const std::string& input) {
        std::string lower = toLower(input);
        int words = wordCount(input);

        // Slash commands
        if (!input.empty() && input[0] == '/') return MKInputType::COMMAND;

        // Greetings
        if (isGreeting(lower)) return MKInputType::GREETING;

        // Goodbyes
        if (isGoodbye(lower)) return MKInputType::GOODBYE;

        // Vague responses
        if (words <= 3) {
            for (const auto& vr : vague_responses_) {
                if (lower == vr || lower == vr + "." || lower == vr + "!") {
                    return MKInputType::VAGUE_RESPONSE;
                }
            }
        }

        // Topic change
        if (words <= 4 && startsWithAny(lower, topic_change_words_)) {
            return MKInputType::TOPIC_CHANGE;
        }

        // Story detection: long + narrative words
        if (words > 15 && countMatches(lower, narrative_words_) >= 2) {
            return MKInputType::STORY;
        }

        // Story indicators at the start
        if (containsAny(lower, story_indicators_)) return MKInputType::STORY;

        // Opinion sharing
        if (startsWithAny(lower, opinion_indicators_)) return MKInputType::OPINION_SHARE;

        // Emotional expression
        MKMoodResult mood = detectMood(input);
        if (mood.confidence >= 0.4f) {
            // Long emotional text = venting
            if (words > 10) return MKInputType::VENT;
            return MKInputType::EMOTIONAL;
        }

        // Has question mark or question words
        if (input.find('?') != std::string::npos) return MKInputType::QUESTION;
        for (const auto& qw : question_words_) {
            if (lower.length() >= qw.length() + 1 &&
                lower.substr(0, qw.length()) == qw &&
                lower[qw.length()] == ' ') {
                return MKInputType::QUESTION;
            }
        }

        // Default for short non-question text
        if (words <= 6) return MKInputType::SMALL_TALK;

        return MKInputType::QUESTION;  // fallback: treat as question
    }

    // ---------------------------------------------------------------
    // IS THIS A STORY / VENT?
    // Long input (>15 words) + narrative flow words
    // ---------------------------------------------------------------
    bool isStory(const std::string& input) {
        std::string lower = toLower(input);
        int words = wordCount(input);

        // Must be long enough
        if (words <= 15) return false;

        // Count narrative words
        int narrativeCount = countMatches(lower, narrative_words_);
        
        // Story if it has narrative flow
        return narrativeCount >= 2;
    }

    // ---------------------------------------------------------------
    // IS THIS AN OPINION REQUEST?
    // "what do you think about X"
    // ---------------------------------------------------------------
    bool isOpinionRequest(const std::string& input) {
        std::string lower = toLower(input);
        return (lower.find("what do you think") != std::string::npos ||
                lower.find("what you think") != std::string::npos ||
                lower.find("your opinion on") != std::string::npos ||
                lower.find("your thoughts on") != std::string::npos ||
                lower.find("how do you feel about") != std::string::npos ||
                lower.find("you fw") != std::string::npos ||
                lower.find("you like") != std::string::npos);
    }

    // ---------------------------------------------------------------
    // EXTRACT OPINION SUBJECT
    // Gets the "X" from "what do you think about X"
    // ---------------------------------------------------------------
    std::string extractOpinionSubject(const std::string& input) {
        std::string lower = toLower(input);
        
        std::vector<std::string> markers = {
            "what do you think about ",
            "what you think about ",
            "your opinion on ",
            "your thoughts on ",
            "how do you feel about ",
            "you fw ",
            "do you like "
        };

        for (const auto& marker : markers) {
            size_t pos = lower.find(marker);
            if (pos != std::string::npos) {
                std::string subject = input.substr(pos + marker.length());
                // Trim trailing punctuation
                while (!subject.empty() && (subject.back() == '?' || subject.back() == '.' || subject.back() == ' ')) {
                    subject.pop_back();
                }
                return subject;
            }
        }
        return "";
    }

    // ---------------------------------------------------------------
    // FOLLOW-UP MEMORY
    // Track last 3 topics discussed
    // ---------------------------------------------------------------
    void pushTopic(const std::string& topic) {
        if (topic.empty()) return;
        if (recentTopics_.size() >= MAX_TOPICS) {
            recentTopics_.pop_front();
        }
        recentTopics_.push_back(topic);
    }

    void resetTopics() {
        recentTopics_.clear();
    }

    std::string getLastTopic() const {
        if (recentTopics_.empty()) return "";
        return recentTopics_.back();
    }

    const std::deque<std::string>& getRecentTopics() const {
        return recentTopics_;
    }

    // ---------------------------------------------------------------
    // GENERATE RESPONSE
    // Main method: given input + casual_responses DB, produce a reply
    // ---------------------------------------------------------------
    std::string generateResponse(const std::string& input, MKCasualResponses& db) {
        MKInputType type = classifyInput(input);
        std::string lower = toLower(input);

        switch (type) {
            case MKInputType::GREETING:
                return db.pick(db.greetings);

            case MKInputType::GOODBYE:
                return db.pick(db.goodbyes);

            case MKInputType::VAGUE_RESPONSE: {
                // Respond in context of last topic
                std::string lastTopic = getLastTopic();
                if (!lastTopic.empty()) {
                    // Acknowledge and reference context
                    return db.pick(db.acknowledgments);
                }
                return db.pick(db.acknowledgments);
            }

            case MKInputType::TOPIC_CHANGE: {
                resetTopics();
                return "what's up?";
            }

            case MKInputType::STORY:
            case MKInputType::VENT: {
                // Long emotional/narrative text — empathize + follow up
                MKMoodResult mood = detectMood(input);
                std::string response;
                
                if (mood.mood == MKMood::SAD || mood.mood == MKMood::ANGRY) {
                    // Empathize first, then follow up
                    if (mood.mood == MKMood::SAD) {
                        response = db.pick(db.mood_sad);
                    } else {
                        response = db.pick(db.mood_angry);
                    }
                    response += ". " + db.pick(db.follow_ups);
                } else {
                    // Story acknowledgment + follow up
                    response = db.pick(db.story_responses);
                }
                
                // Track topic
                pushTopic(input.substr(0, std::min((size_t)50, input.length())));
                return response;
            }

            case MKInputType::OPINION_SHARE: {
                // User sharing their opinion — acknowledge it
                MKMoodResult mood = detectMood(input);
                if (mood.mood == MKMood::HAPPY || mood.confidence < 0.3f) {
                    return db.pick(db.acknowledgments) + ". " + db.pick(db.reactions_positive);
                } else if (mood.mood == MKMood::SAD || mood.mood == MKMood::ANGRY) {
                    return db.pick(db.acknowledgments) + ". " + db.pick(db.reactions_negative);
                }
                return db.pick(db.acknowledgments);
            }

            case MKInputType::EMOTIONAL: {
                // Short emotional expression — respond to mood
                MKMoodResult mood = detectMood(input);
                switch (mood.mood) {
                    case MKMood::HAPPY:   return db.pick(db.mood_happy);
                    case MKMood::SAD:     return db.pick(db.mood_sad);
                    case MKMood::ANGRY:   return db.pick(db.mood_angry);
                    case MKMood::NERVOUS: return db.pick(db.mood_nervous);
                    case MKMood::CHILL:   return db.pick(db.mood_chill);
                    default:              return db.pick(db.acknowledgments);
                }
            }

            case MKInputType::SMALL_TALK: {
                // Generic short statement
                MKMoodResult mood = detectMood(input);
                if (mood.confidence >= 0.3f) {
                    switch (mood.mood) {
                        case MKMood::HAPPY:   return db.pick(db.mood_happy);
                        case MKMood::SAD:     return db.pick(db.mood_sad);
                        case MKMood::ANGRY:   return db.pick(db.mood_angry);
                        case MKMood::NERVOUS: return db.pick(db.mood_nervous);
                        case MKMood::CHILL:   return db.pick(db.mood_chill);
                        default: break;
                    }
                }
                // No strong mood detected — give a chill response
                return db.pick(db.mood_chill);
            }

            default:
                return "";  // Not conversation — let normal routing handle it
        }
    }

    // ---------------------------------------------------------------
    // GENERATE OPINION (uses knowledge graph data)
    // ---------------------------------------------------------------
    std::string generateOpinion(const std::string& subject,
                                const std::vector<std::string>& facts,
                                MKCasualResponses& db) {
        if (facts.empty()) {
            // No data on this topic
            return "hmm idk enough about " + subject + " to have an opinion yet. teach me?";
        }

        // Simple heuristic: check if facts contain positive or negative words
        std::vector<std::string> positive_words = {
            "good", "great", "best", "fast", "popular", "useful", "powerful",
            "efficient", "easy", "reliable", "strong", "successful"
        };
        std::vector<std::string> negative_words = {
            "bad", "slow", "difficult", "complex", "buggy", "deprecated",
            "outdated", "weak", "unreliable", "problematic", "limited"
        };

        int positive_score = 0;
        int negative_score = 0;

        for (const auto& fact : facts) {
            std::string factLower = toLower(fact);
            for (const auto& pw : positive_words) {
                if (factLower.find(pw) != std::string::npos) positive_score++;
            }
            for (const auto& nw : negative_words) {
                if (factLower.find(nw) != std::string::npos) negative_score++;
            }
        }

        std::string firstFact = facts[0];
        // Keep it short
        if (firstFact.length() > 60) {
            firstFact = firstFact.substr(0, 57) + "...";
        }

        if (positive_score > negative_score) {
            return db.pick(db.opinions_positive) + ". from what I know: " + firstFact;
        } else if (negative_score > positive_score) {
            return db.pick(db.opinions_negative) + ". based on what I got: " + firstFact;
        } else {
            return "honestly I'm kinda neutral on " + subject + ". I know that " + firstFact + " but idk enough to pick a side";
        }
    }
};

#endif // MK_CONVERSATION_MODE_CPP
