#ifndef MK_PERSONALITY_ENGINE_CPP
#define MK_PERSONALITY_ENGINE_CPP

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <cstdlib>

// ===========================================================================
// MK PERSONALITY ENGINE - Dynamic Response Personality System
// ===========================================================================
// Defines MK's personality traits and adapts response style based on
// user interaction patterns. 5 preset personalities with dynamic blending.
// Traits: helpfulness, humor, formality, verbosity, curiosity
// Presets: Friendly, Technical, Teacher, Creative, Concise
// ===========================================================================

// Personality trait values (0.0 to 1.0)
struct MKPersonalityTraits {
    float helpfulness;    // How eager to help (0=minimal, 1=very proactive)
    float humor;          // How much humor to inject (0=serious, 1=playful)
    float formality;      // How formal (0=casual, 1=very formal)
    float verbosity;      // How verbose (0=brief, 1=detailed)
    float curiosity;      // How much to ask follow-ups (0=none, 1=lots)
    float empathy;        // Emotional awareness (0=robotic, 1=warm)
    float confidence;     // How confident in responses (0=hedging, 1=assertive)
};

// Mood state that changes over conversation
enum class MKMoodState {
    NEUTRAL,
    ENTHUSIASTIC,
    FOCUSED,
    PATIENT,
    CURIOUS,
    APOLOGETIC
};

// Personality preset
struct MKPersonalityPreset {
    std::string name;
    std::string description;
    MKPersonalityTraits traits;
    std::vector<std::string> greetings;
    std::vector<std::string> acknowledgments;
    std::vector<std::string> transitions;
    std::vector<std::string> closings;
};

// Response style modifiers
struct MKResponseStyle {
    bool use_emoji;
    bool use_analogies;
    bool add_examples;
    bool ask_followup;
    int max_sentences;
    std::string tone;  // casual, professional, academic, friendly
};

class MKPersonalityEngine {
private:
    MKPersonalityTraits current_traits;
    MKMoodState current_mood;
    std::vector<MKPersonalityPreset> presets;
    std::string active_preset;
    int interaction_count;
    float user_expertise_estimate;  // 0=beginner, 1=expert

    void initPresets() {
        // Friendly: warm, casual, helpful
        presets.push_back({
            "friendly", "Warm and approachable, like a helpful friend",
            {0.9f, 0.6f, 0.2f, 0.6f, 0.7f, 0.9f, 0.7f},
            {"Hey! What can I help you with?", "Hi there! Ready to help.",
             "Good to see you! What's on your mind?"},
            {"Got it!", "Sure thing!", "Absolutely!", "No problem!"},
            {"By the way,", "Oh, and", "Also worth noting:"},
            {"Let me know if you need anything else!",
             "Happy to help more if needed!", "Anytime!"}
        });

        // Technical: precise, formal, detailed
        presets.push_back({
            "technical", "Precise and detailed, focuses on accuracy",
            {0.8f, 0.1f, 0.8f, 0.9f, 0.4f, 0.3f, 0.9f},
            {"How may I assist you?", "Ready for your query.",
             "Please state your question."},
            {"Understood.", "Acknowledged.", "Noted.", "Processing."},
            {"Furthermore,", "Additionally,", "It should be noted that"},
            {"Is there anything else you need clarification on?",
             "Shall I elaborate on any point?", "Query complete."}
        });

        // Teacher: patient, explanatory, encouraging
        presets.push_back({
            "teacher", "Patient educator who explains concepts clearly",
            {0.9f, 0.3f, 0.4f, 0.8f, 0.8f, 0.7f, 0.6f},
            {"Great question! Let me explain.", "Let's explore this together.",
             "I'd love to help you understand this."},
            {"That's a good start!", "You're on the right track.",
             "Exactly! And building on that..."},
            {"Think of it like this:", "To put it simply:",
             "Here's an analogy that might help:"},
            {"Does that make sense? Feel free to ask more!",
             "Want me to go deeper on any part?",
             "Try it out and let me know how it goes!"}
        });

        // Creative: playful, uses metaphors, thinks outside the box
        presets.push_back({
            "creative", "Imaginative and playful, loves metaphors",
            {0.8f, 0.8f, 0.1f, 0.7f, 0.9f, 0.6f, 0.6f},
            {"Ooh, interesting challenge!", "Let's get creative with this!",
             "I've got ideas brewing already!"},
            {"Love it!", "Now we're cooking!", "That's the spirit!"},
            {"Picture this:", "Imagine if", "What if we think of it as"},
            {"That was fun! Bring me another puzzle!",
             "My creative circuits are buzzing!",
             "Let's brainstorm more sometime!"}
        });

        // Concise: minimal, direct, efficient
        presets.push_back({
            "concise", "Brief and to the point, no fluff",
            {0.7f, 0.0f, 0.5f, 0.1f, 0.2f, 0.2f, 0.9f},
            {"Yes?", "Go ahead.", "Listening."},
            {"Done.", "OK.", "Right."},
            {"", "Also:", "And:"},
            {"Done.", "Next?", "Anything else?"}
        });
    }

    // Get a random element from a vector
    std::string randomChoice(const std::vector<std::string>& choices) {
        if (choices.empty()) return "";
        return choices[std::rand() % choices.size()];
    }

    // Find preset by name
    const MKPersonalityPreset* findPreset(const std::string& name) {
        for (const auto& p : presets) {
            if (p.name == name) return &p;
        }
        return nullptr;
    }

    // Determine mood from conversation context
    MKMoodState inferMood(const std::string& user_input) {
        std::string lower = user_input;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        if (lower.find("sorry") != std::string::npos ||
            lower.find("mistake") != std::string::npos) return MKMoodState::APOLOGETIC;
        if (lower.find("awesome") != std::string::npos ||
            lower.find("cool") != std::string::npos ||
            lower.find("great") != std::string::npos) return MKMoodState::ENTHUSIASTIC;
        if (lower.find("how") != std::string::npos ||
            lower.find("why") != std::string::npos ||
            lower.find("what") != std::string::npos) return MKMoodState::CURIOUS;
        if (lower.find("help") != std::string::npos ||
            lower.find("stuck") != std::string::npos) return MKMoodState::PATIENT;
        if (lower.find("code") != std::string::npos ||
            lower.find("debug") != std::string::npos ||
            lower.find("fix") != std::string::npos) return MKMoodState::FOCUSED;
        return MKMoodState::NEUTRAL;
    }

    // Estimate user expertise from their language
    float estimateExpertise(const std::string& input) {
        std::string lower = input;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        float score = 0.5f;

        // Technical terms suggest expertise
        std::vector<std::string> expert_terms = {
            "algorithm", "complexity", "architecture", "refactor",
            "polymorphism", "concurrency", "deadlock", "middleware",
            "microservice", "kubernetes", "terraform", "latency"
        };
        std::vector<std::string> beginner_terms = {
            "simple", "basic", "beginner", "how do i",
            "what is", "explain", "new to", "first time"
        };

        for (const auto& term : expert_terms)
            if (lower.find(term) != std::string::npos) score += 0.1f;
        for (const auto& term : beginner_terms)
            if (lower.find(term) != std::string::npos) score -= 0.1f;

        return std::max(0.0f, std::min(1.0f, score));
    }

public:
    MKPersonalityEngine() : current_mood(MKMoodState::NEUTRAL),
                             interaction_count(0), user_expertise_estimate(0.5f) {
        std::srand(static_cast<unsigned>(std::time(nullptr)));
        initPresets();
        setPreset("friendly");  // Default personality
    }

    // Set active personality preset
    bool setPreset(const std::string& name) {
        const auto* preset = findPreset(name);
        if (!preset) return false;
        active_preset = name;
        current_traits = preset->traits;
        return true;
    }

    // Get a greeting appropriate to current personality
    std::string getGreeting() {
        const auto* preset = findPreset(active_preset);
        if (!preset) return "Hello.";
        return randomChoice(preset->greetings);
    }

    // Get an acknowledgment
    std::string getAcknowledgment() {
        const auto* preset = findPreset(active_preset);
        if (!preset) return "OK.";
        return randomChoice(preset->acknowledgments);
    }

    // Get a transition phrase
    std::string getTransition() {
        const auto* preset = findPreset(active_preset);
        if (!preset) return "";
        return randomChoice(preset->transitions);
    }

    // Get a closing phrase
    std::string getClosing() {
        const auto* preset = findPreset(active_preset);
        if (!preset) return "";
        return randomChoice(preset->closings);
    }

    // Determine response style based on context
    MKResponseStyle getResponseStyle(const std::string& user_input) {
        interaction_count++;
        current_mood = inferMood(user_input);
        user_expertise_estimate = estimateExpertise(user_input);

        MKResponseStyle style;
        style.use_emoji = (current_traits.humor > 0.5f);
        style.use_analogies = (current_traits.verbosity > 0.5f && user_expertise_estimate < 0.6f);
        style.add_examples = (current_traits.helpfulness > 0.6f);
        style.ask_followup = (current_traits.curiosity > 0.5f);
        style.max_sentences = static_cast<int>(current_traits.verbosity * 10) + 2;

        if (current_traits.formality > 0.7f) style.tone = "professional";
        else if (current_traits.formality > 0.4f) style.tone = "friendly";
        else style.tone = "casual";

        return style;
    }

    // Add personality flavor to a raw response
    std::string addFlavor(const std::string& raw_response, const std::string& user_input) {
        auto style = getResponseStyle(user_input);
        std::string flavored;

        // Add acknowledgment for short user inputs
        if (user_input.length() < 50 && current_traits.empathy > 0.5f) {
            flavored = getAcknowledgment() + " ";
        }

        flavored += raw_response;

        // Add follow-up question if curious personality
        if (style.ask_followup && interaction_count % 3 == 0) {
            flavored += "\n\n" + getClosing();
        }

        return flavored;
    }

    // Get current personality info
    std::string getActivePreset() const { return active_preset; }
    MKPersonalityTraits getTraits() const { return current_traits; }
    MKMoodState getMood() const { return current_mood; }
    int getInteractionCount() const { return interaction_count; }

    // List available presets
    std::vector<std::string> listPresets() {
        std::vector<std::string> names;
        for (const auto& p : presets) names.push_back(p.name);
        return names;
    }

    void printStats() const {
        std::cout << "[MKPersonalityEngine] Preset: " << active_preset
                  << ", Interactions: " << interaction_count << std::endl;
    }
};

#endif // MK_PERSONALITY_ENGINE_CPP
