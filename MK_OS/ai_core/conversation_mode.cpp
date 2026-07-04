#ifndef MK_CONVERSATION_MODE_CPP
#define MK_CONVERSATION_MODE_CPP

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>
#include <deque>
#include <ctime>

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

// Emotion intensity level
enum class MKEmotionIntensity {
    MILD,       // Slightly feeling something
    MODERATE,   // Clearly emotional
    STRONG,     // Very emotional
    EXTREME     // Off the charts
};

// Result of emotion detection
struct MKMoodResult {
    MKMood mood;
    float confidence;  // 0.0 - 1.0
    MKEmotionIntensity intensity;  // How strong the emotion is
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

// Time of day categories
enum class MKTimeOfDay {
    EARLY_MORNING,   // 4am - 7am
    MORNING,         // 7am - 12pm
    AFTERNOON,       // 12pm - 5pm
    EVENING,         // 5pm - 9pm
    NIGHT,           // 9pm - 12am
    LATE_NIGHT       // 12am - 4am
};

class MKConversationMode {
private:
    std::mt19937 rng_;
    
    // Follow-up memory: last 3 topics discussed
    std::deque<std::string> recentTopics_;
    static const size_t MAX_TOPICS = 3;

    // Story mode tracking
    bool inStoryMode_ = false;
    int storyTurnCount_ = 0;
    
    // Debate mode tracking
    bool debateMode_ = false;
    int debateChance_ = 25;  // % chance to disagree for fun
    
    // Conversation memory across sessions
    std::vector<std::string> sessionTopics_;
    int totalTurns_ = 0;

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
        "yeah", "yes", "true", "exactly", "fr", "facts", "real",
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
        result.intensity = MKEmotionIntensity::MILD;
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

        // Determine emotion intensity based on score and confidence
        if (max_score >= 4 || result.confidence >= 0.9f) {
            result.intensity = MKEmotionIntensity::EXTREME;
        } else if (max_score >= 3 || result.confidence >= 0.7f) {
            result.intensity = MKEmotionIntensity::STRONG;
        } else if (max_score >= 2 || result.confidence >= 0.4f) {
            result.intensity = MKEmotionIntensity::MODERATE;
        } else {
            result.intensity = MKEmotionIntensity::MILD;
        }

        // Caps lock / exclamation boost intensity
        int capsCount = 0;
        int exclCount = 0;
        for (char c : input) {
            if (c >= 'A' && c <= 'Z') capsCount++;
            if (c == '!') exclCount++;
        }
        if (capsCount > 5 || exclCount >= 2) {
            if (result.intensity == MKEmotionIntensity::MILD) result.intensity = MKEmotionIntensity::MODERATE;
            else if (result.intensity == MKEmotionIntensity::MODERATE) result.intensity = MKEmotionIntensity::STRONG;
            else if (result.intensity == MKEmotionIntensity::STRONG) result.intensity = MKEmotionIntensity::EXTREME;
            result.confidence = std::min(1.0f, result.confidence + 0.15f);
        }

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
    // TIME OF DAY AWARENESS
    // Returns what time bracket we're in
    // ---------------------------------------------------------------
    MKTimeOfDay getTimeOfDay() const {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        struct tm* timeinfo = localtime(&time_t_now);
        int hour = timeinfo->tm_hour;

        if (hour >= 4 && hour < 7) return MKTimeOfDay::EARLY_MORNING;
        if (hour >= 7 && hour < 12) return MKTimeOfDay::MORNING;
        if (hour >= 12 && hour < 17) return MKTimeOfDay::AFTERNOON;
        if (hour >= 17 && hour < 21) return MKTimeOfDay::EVENING;
        if (hour >= 21) return MKTimeOfDay::NIGHT;
        return MKTimeOfDay::LATE_NIGHT;  // 0am - 4am
    }

    // ---------------------------------------------------------------
    // TIME-AWARE GREETING
    // Returns an appropriate greeting based on time of day
    // ---------------------------------------------------------------
    std::string getTimeAwareGreeting(MKCasualResponses& db) {
        MKTimeOfDay tod = getTimeOfDay();
        switch (tod) {
            case MKTimeOfDay::EARLY_MORNING:
                return "you're up early! what can I help with?";
            case MKTimeOfDay::MORNING:
                return "good morning! what's on your mind?";
            case MKTimeOfDay::AFTERNOON:
                return "hey, how's your afternoon going?";
            case MKTimeOfDay::EVENING:
                return "good evening. what can I do for you?";
            case MKTimeOfDay::NIGHT:
                return "hey, you're up late. what's on your mind?";
            case MKTimeOfDay::LATE_NIGHT:
                return "it's pretty late. everything good?";
        }
        return db.pick(db.greetings);
    }

    // ---------------------------------------------------------------
    // STORY MODE DETECTION
    // Detects when user is telling a long story and responds accordingly
    // ---------------------------------------------------------------
    bool detectStoryMode(const std::string& input) {
        std::string lower = toLower(input);
        int words = wordCount(input);

        // Already in story mode - check if story continues
        if (inStoryMode_) {
            // Stories continue if text is longer than a quick response
            if (words > 8) {
                storyTurnCount_++;
                return true;
            }
            // Short response might end the story
            if (words <= 3) {
                inStoryMode_ = false;
                storyTurnCount_ = 0;
                return false;
            }
        }

        // Enter story mode if long narrative detected
        if (words > 20 && countMatches(lower, narrative_words_) >= 3) {
            inStoryMode_ = true;
            storyTurnCount_ = 1;
            return true;
        }

        // Story indicators
        if (words > 10 && containsAny(lower, story_indicators_)) {
            inStoryMode_ = true;
            storyTurnCount_ = 1;
            return true;
        }

        return false;
    }

    // ---------------------------------------------------------------
    // STORY MODE RESPONSE
    // Generate engaged follow-ups for ongoing stories
    // ---------------------------------------------------------------
    std::string generateStoryResponse(const std::string& input, MKCasualResponses& db) {
        MKMoodResult mood = detectMood(input);
        std::string response;

        // First turn of story: excited reaction
        if (storyTurnCount_ <= 1) {
            response = db.pick(db.story_responses);
        }
        // Middle of story: show engagement
        else if (storyTurnCount_ <= 3) {
            if (mood.mood == MKMood::SAD || mood.mood == MKMood::ANGRY) {
                response = (mood.mood == MKMood::SAD) ?
                    db.pick(db.mood_sad) : db.pick(db.mood_angry);
                response += ". " + db.pick(db.follow_ups);
            } else {
                response = db.pick(db.follow_ups);
            }
        }
        // Long story: react with intensity matching their emotion
        else {
            if (mood.intensity == MKEmotionIntensity::EXTREME ||
                mood.intensity == MKEmotionIntensity::STRONG) {
                response = "bro this story is INSANE. ";
                response += db.pick(db.follow_ups);
            } else {
                response = db.pick(db.curiosity);
            }
        }

        pushTopic(input.substr(0, std::min((size_t)50, input.length())));
        return response;
    }

    // ---------------------------------------------------------------
    // DEBATE MODE
    // Sometimes respectfully disagree to keep things interesting
    // ---------------------------------------------------------------
    bool shouldDebate(const std::string& input) {
        std::string lower = toLower(input);

        // Only debate opinion statements
        if (!containsAny(lower, opinion_indicators_)) return false;

        // Random chance
        std::uniform_int_distribution<int> dist(1, 100);
        return dist(rng_) <= debateChance_;
    }

    std::string generateDebateResponse(const std::string& input, MKCasualResponses& db) {
        (void)input;
        std::vector<std::string> debate_starters = {
            "hmm interesting take but hear me out",
            "ok I respect that but counter-argument",
            "ngl I see it differently but go on",
            "that's valid but what about the other side tho",
            "ok hot take incoming but I kinda disagree",
            "respectfully I gotta push back on that one",
            "I hear you but let me play devil's advocate",
            "that's one way to look at it but consider this",
            "ok I'm not sure I'm fully on board with that",
            "interesting point but I think there's more to it"
        };
        std::uniform_int_distribution<size_t> dist(0, debate_starters.size() - 1);
        std::string response = debate_starters[dist(rng_)];
        response += ". " + db.pick(db.curiosity);
        return response;
    }

    // ---------------------------------------------------------------
    // EMOTION INTENSITY RESPONSE
    // Adjust response based on how strong the emotion is
    // ---------------------------------------------------------------
    std::string getIntensityResponse(MKMoodResult& mood, MKCasualResponses& db) {
        std::string base;
        switch (mood.mood) {
            case MKMood::HAPPY:   base = db.pick(db.mood_happy); break;
            case MKMood::SAD:     base = db.pick(db.mood_sad); break;
            case MKMood::ANGRY:   base = db.pick(db.mood_angry); break;
            case MKMood::NERVOUS: base = db.pick(db.mood_nervous); break;
            case MKMood::CHILL:   base = db.pick(db.mood_chill); break;
            default:              base = db.pick(db.acknowledgments); break;
        }

        // Add intensity modifiers
        if (mood.intensity == MKEmotionIntensity::EXTREME) {
            if (mood.mood == MKMood::HAPPY) {
                base += "!! bro the energy is OFF THE CHARTS. " + db.pick(db.hype_up);
            } else if (mood.mood == MKMood::SAD) {
                base += ". bro I'm genuinely worried, I'm here for you 100%. " + db.pick(db.encouragements);
            } else if (mood.mood == MKMood::ANGRY) {
                base += ". nah bro that's UNACCEPTABLE fr. you want to talk about it?";
            }
        } else if (mood.intensity == MKEmotionIntensity::STRONG) {
            if (mood.mood == MKMood::HAPPY) {
                base += ". " + db.pick(db.reactions_positive);
            } else if (mood.mood == MKMood::SAD || mood.mood == MKMood::ANGRY) {
                base += ". " + db.pick(db.encouragements);
            }
        }

        return base;
    }

    // ---------------------------------------------------------------
    // GENERATE RESPONSE
    // Main method: given input + casual_responses DB, produce a reply
    // ---------------------------------------------------------------
    std::string generateResponse(const std::string& input, MKCasualResponses& db) {
        MKInputType type = classifyInput(input);
        std::string lower = toLower(input);
        totalTurns_++;

        // Check for story mode first (overrides normal classification)
        if (detectStoryMode(input)) {
            return generateStoryResponse(input, db);
        }

        // Check for debate mode on opinion shares
        if (type == MKInputType::OPINION_SHARE && shouldDebate(input)) {
            return generateDebateResponse(input, db);
        }

        switch (type) {
            case MKInputType::GREETING: {
                // Use time-aware greeting sometimes
                std::uniform_int_distribution<int> dist(1, 3);
                if (dist(rng_) == 1) {
                    return getTimeAwareGreeting(db);
                }
                return db.pick(db.greetings);
            }

            case MKInputType::GOODBYE:
                return db.pick(db.goodbyes);

            case MKInputType::VAGUE_RESPONSE: {
                // Respond in context of last topic
                std::string lastTopic = getLastTopic();
                if (!lastTopic.empty()) {
                    // Reference the last topic so user knows context is preserved
                    std::vector<std::string> contextResponses = {
                        "got it. want me to tell you more about " + lastTopic + "?",
                        "yeah, " + lastTopic + " is interesting. want to go deeper?",
                        "I can tell you more about " + lastTopic + " if you want.",
                        "right. anything else about " + lastTopic + "?",
                        "cool, let me know if you have more questions about " + lastTopic + "."
                    };
                    std::uniform_int_distribution<size_t> dist(0, contextResponses.size() - 1);
                    return contextResponses[dist(rng_)];
                }
                return db.pick(db.acknowledgments);
            }

            case MKInputType::TOPIC_CHANGE: {
                resetTopics();
                MKTimeOfDay tod = getTimeOfDay();
                if (tod == MKTimeOfDay::LATE_NIGHT || tod == MKTimeOfDay::NIGHT) {
                    return "what's on your mind this late?";
                }
                return "what's up?";
            }

            case MKInputType::STORY:
            case MKInputType::VENT: {
                // Long emotional/narrative text -- empathize + follow up
                MKMoodResult mood = detectMood(input);
                std::string response;
                
                if (mood.intensity == MKEmotionIntensity::EXTREME ||
                    mood.intensity == MKEmotionIntensity::STRONG) {
                    response = getIntensityResponse(mood, db);
                } else if (mood.mood == MKMood::SAD || mood.mood == MKMood::ANGRY) {
                    if (mood.mood == MKMood::SAD) {
                        response = db.pick(db.mood_sad);
                    } else {
                        response = db.pick(db.mood_angry);
                    }
                    response += ". " + db.pick(db.follow_ups);
                } else {
                    response = db.pick(db.story_responses);
                }
                
                // Track topic
                pushTopic(input.substr(0, std::min((size_t)50, input.length())));
                sessionTopics_.push_back(input.substr(0, std::min((size_t)30, input.length())));
                return response;
            }

            case MKInputType::OPINION_SHARE: {
                // User sharing their opinion -- acknowledge it
                MKMoodResult mood = detectMood(input);
                if (mood.mood == MKMood::HAPPY || mood.confidence < 0.3f) {
                    return db.pick(db.acknowledgments) + ". " + db.pick(db.reactions_positive);
                } else if (mood.mood == MKMood::SAD || mood.mood == MKMood::ANGRY) {
                    return db.pick(db.acknowledgments) + ". " + db.pick(db.reactions_negative);
                }
                return db.pick(db.acknowledgments);
            }

            case MKInputType::EMOTIONAL: {
                // Short emotional expression -- respond with intensity awareness
                MKMoodResult mood = detectMood(input);
                return getIntensityResponse(mood, db);
            }

            case MKInputType::SMALL_TALK: {
                // Generic short statement
                MKMoodResult mood = detectMood(input);
                if (mood.confidence >= 0.3f) {
                    return getIntensityResponse(mood, db);
                }
                // No strong mood detected -- give a chill response
                return db.pick(db.mood_chill);
            }

            default:
                return "";  // Not conversation -- let normal routing handle it
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

    // ---------------------------------------------------------------
    // STATE ACCESSORS for new features
    // ---------------------------------------------------------------
    bool isInStoryMode() const { return inStoryMode_; }
    bool isInDebateMode() const { return debateMode_; }
    int getStoryTurnCount() const { return storyTurnCount_; }
    int getTotalTurns() const { return totalTurns_; }
    const std::vector<std::string>& getSessionTopics() const { return sessionTopics_; }

    void setDebateChance(int percent) {
        debateChance_ = std::max(0, std::min(100, percent));
    }
};

#endif // MK_CONVERSATION_MODE_CPP
