#ifndef MK_PERSONALITY_CPP
#define MK_PERSONALITY_CPP

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <cstdlib>

// ============================================================================
// MKPersonality - Defines MK's character and tone
// Slightly casual, helpful, honest about limitations, occasionally witty
// Time-aware greetings and a mood system
// ============================================================================

enum class Mood {
    NEUTRAL,
    ENTHUSIASTIC,
    THOUGHTFUL,
    PLAYFUL,
    FOCUSED,
    TIRED
};

enum class ToneStyle {
    DEFAULT,
    CASUAL,
    PROFESSIONAL,
    ENTHUSIASTIC,
    CONCISE
};

struct PersonalityTraits {
    float helpfulness;      // 0.0 - 1.0
    float casualness;       // 0.0 - 1.0
    float wit_level;        // 0.0 - 1.0
    float verbosity;        // 0.0 - 1.0
    float honesty;          // 0.0 - 1.0 (about limitations)
};

class MKPersonality {
private:
    PersonalityTraits traits_;
    Mood current_mood_;
    ToneStyle current_tone_;
    int interaction_count_;
    int positive_interactions_;
    int total_session_messages_;
    std::string user_name_;

    std::vector<std::string> greetings_morning_;
    std::vector<std::string> greetings_afternoon_;
    std::vector<std::string> greetings_evening_;
    std::vector<std::string> greetings_night_;
    std::vector<std::string> uncertain_responses_;
    std::vector<std::string> witty_additions_;
    std::vector<std::string> encouragements_;
    std::vector<std::string> transitions_;

    void initializeResponses() {
        greetings_morning_ = {
            "Good morning! Ready to tackle some problems today?",
            "Morning! What can I help you with?",
            "Hey, good morning! Let's make today productive.",
            "Rise and shine! What's on your mind?",
            "Good morning! Coffee first, or shall we dive right in?"
        };
        greetings_afternoon_ = {
            "Good afternoon! How can I help?",
            "Hey there! What are you working on?",
            "Afternoon! Ready when you are.",
            "Hi! Hope your day's going well. What do you need?",
            "Good afternoon! Let's get something done."
        };
        greetings_evening_ = {
            "Good evening! Still going strong?",
            "Hey! Working late? I'm here to help.",
            "Evening! What can I do for you?",
            "Good evening! Let's make this productive.",
            "Hi there! Burning the midnight oil?"
        };
        greetings_night_ = {
            "Hey, burning the midnight oil? I'm here if you need me.",
            "Late night coding session? I'm ready when you are.",
            "Still up? Let's figure this out together.",
            "Night owl mode activated. What do you need?",
            "It's late, but I'm always here. What's up?"
        };
        uncertain_responses_ = {
            "Hmm, I'm not entirely sure about that. Let me think...",
            "That's a good question - I don't have a definitive answer, but here's what I think:",
            "I'll be honest, I'm not 100% confident here, but my best take is:",
            "I might be wrong on this, but from what I understand:",
            "I don't want to mislead you - I'm somewhat uncertain, but:",
            "Let me be upfront: I'm not sure, but I can share my reasoning:",
            "This is outside my strongest area, but here's my attempt:"
        };
        witty_additions_ = {
            " (No pressure, but that was pretty cool.)",
            " ...I'd high-five you if I had hands.",
            " Not bad for a day's work!",
            " And they said it couldn't be done. Well, nobody said that, but still.",
            " That's what we in the business call 'nailing it'.",
            " *chef's kiss*",
            " I love it when a plan comes together."
        };
        encouragements_ = {
            "You're on the right track!",
            "That's a solid approach.",
            "Good thinking!",
            "You're making great progress.",
            "Nice work so far!",
            "That's a clever solution."
        };
        transitions_ = {
            "Alright, ",
            "So, ",
            "Let's see... ",
            "Here's the thing: ",
            "Okay, ",
            "Right, "
        };
    }

    std::string getTimeOfDay() const {
        time_t now = time(nullptr);
        struct tm* local = localtime(&now);
        int hour = local->tm_hour;
        if (hour >= 5 && hour < 12) return "morning";
        if (hour >= 12 && hour < 17) return "afternoon";
        if (hour >= 17 && hour < 21) return "evening";
        return "night";
    }

    std::string pickRandom(const std::vector<std::string>& options) const {
        if (options.empty()) return "";
        size_t index = (static_cast<size_t>(interaction_count_) * 7 + 
                       static_cast<size_t>(total_session_messages_) * 13) % options.size();
        return options[index];
    }

    void updateMood() {
        if (total_session_messages_ > 50) {
            current_mood_ = Mood::TIRED;
        } else if (positive_interactions_ > interaction_count_ / 2) {
            current_mood_ = Mood::ENTHUSIASTIC;
        } else if (total_session_messages_ > 20) {
            current_mood_ = Mood::FOCUSED;
        } else if (interaction_count_ < 5) {
            current_mood_ = Mood::PLAYFUL;
        } else {
            current_mood_ = Mood::NEUTRAL;
        }
    }

    float getMoodModifier() const {
        switch (current_mood_) {
            case Mood::ENTHUSIASTIC: return 1.3f;
            case Mood::PLAYFUL: return 1.2f;
            case Mood::FOCUSED: return 0.8f;
            case Mood::TIRED: return 0.6f;
            case Mood::THOUGHTFUL: return 0.9f;
            default: return 1.0f;
        }
    }

public:
    MKPersonality() : current_mood_(Mood::NEUTRAL), current_tone_(ToneStyle::DEFAULT),
                      interaction_count_(0), positive_interactions_(0), 
                      total_session_messages_(0) {
        traits_.helpfulness = 0.95f;
        traits_.casualness = 0.65f;
        traits_.wit_level = 0.4f;
        traits_.verbosity = 0.6f;
        traits_.honesty = 0.95f;
        initializeResponses();
    }

    std::string addFlavor(const std::string& response) {
        total_session_messages_++;
        updateMood();

        std::string flavored = response;
        float mood_mod = getMoodModifier();

        // Add transition word occasionally
        if (traits_.casualness > 0.5f && total_session_messages_ % 3 == 0) {
            std::string transition = pickRandom(transitions_);
            if (!response.empty() && std::islower(response[0])) {
                flavored = transition + flavored;
            }
        }

        // Add wit occasionally based on mood and trait
        if (traits_.wit_level * mood_mod > 0.5f && total_session_messages_ % 7 == 0) {
            flavored += pickRandom(witty_additions_);
        }

        return flavored;
    }

    std::string getGreeting() {
        interaction_count_++;
        std::string time_of_day = getTimeOfDay();
        std::string greeting;

        if (time_of_day == "morning") greeting = pickRandom(greetings_morning_);
        else if (time_of_day == "afternoon") greeting = pickRandom(greetings_afternoon_);
        else if (time_of_day == "evening") greeting = pickRandom(greetings_evening_);
        else greeting = pickRandom(greetings_night_);

        if (!user_name_.empty()) {
            size_t excl = greeting.find('!');
            if (excl != std::string::npos) {
                greeting.insert(excl, ", " + user_name_);
            }
        }
        return greeting;
    }

    std::string getUncertainResponse() {
        return pickRandom(uncertain_responses_);
    }

    std::string adjustTone(ToneStyle style) {
        current_tone_ = style;
        switch (style) {
            case ToneStyle::CASUAL:
                traits_.casualness = 0.9f;
                traits_.verbosity = 0.7f;
                return "Got it, I'll keep things casual.";
            case ToneStyle::PROFESSIONAL:
                traits_.casualness = 0.2f;
                traits_.verbosity = 0.8f;
                return "Understood. I'll maintain a professional tone.";
            case ToneStyle::ENTHUSIASTIC:
                traits_.casualness = 0.7f;
                traits_.wit_level = 0.6f;
                return "Awesome! Let's bring some energy to this!";
            case ToneStyle::CONCISE:
                traits_.verbosity = 0.3f;
                traits_.casualness = 0.4f;
                return "Brief mode on.";
            default:
                traits_.casualness = 0.65f;
                traits_.verbosity = 0.6f;
                return "Back to my usual self.";
        }
    }

    void setUserName(const std::string& name) { user_name_ = name; }
    std::string getUserName() const { return user_name_; }

    void registerPositiveInteraction() { positive_interactions_++; interaction_count_++; }
    void registerNeutralInteraction() { interaction_count_++; }

    Mood getMood() const { return current_mood_; }
    std::string getMoodName() const {
        switch (current_mood_) {
            case Mood::NEUTRAL: return "neutral";
            case Mood::ENTHUSIASTIC: return "enthusiastic";
            case Mood::THOUGHTFUL: return "thoughtful";
            case Mood::PLAYFUL: return "playful";
            case Mood::FOCUSED: return "focused";
            case Mood::TIRED: return "tired";
            default: return "unknown";
        }
    }

    PersonalityTraits getTraits() const { return traits_; }
    ToneStyle getTone() const { return current_tone_; }

    std::string getEncouragement() { return pickRandom(encouragements_); }

    void resetSession() {
        total_session_messages_ = 0;
        current_mood_ = Mood::NEUTRAL;
        current_tone_ = ToneStyle::DEFAULT;
    }
};

#endif // MK_PERSONALITY_CPP
