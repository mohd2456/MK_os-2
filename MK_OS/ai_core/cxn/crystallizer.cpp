#ifndef MK_CXN_CRYSTALLIZER_CPP
#define MK_CXN_CRYSTALLIZER_CPP

#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <algorithm>
#include <random>

// ===================================================================================
// MK CXN ENGINE - Crystallizer
// ===================================================================================
// Fills crystal pattern slots with actual words based on context (subject, emotion,
// tone, facts from knowledge graph, user vocab). Can fuse multiple crystals together
// and handles feedback to improve crystal confidence.
// ===================================================================================

#include "resonance.cpp"

struct MKCrystallizeContext {
    std::string subject;
    std::string emotion;
    std::string tone;        // casual, formal, playful, serious
    std::vector<std::string> facts;       // relevant facts from knowledge graph
    std::vector<std::string> userVocab;   // words the user tends to use
    std::string originalInput;
};

class MKCrystallizer {
private:
    std::mt19937 rng_;

    // Vocabulary pools for slot filling
    std::unordered_map<std::string, std::vector<std::string>> slotVocab_;

    // Initialize default vocabulary for common slot types
    void initDefaultVocab() {
        slotVocab_["subject"] = {"it", "that", "this topic", "the concept", "this idea"};
        slotVocab_["object"] = {"something", "a lot", "many things", "quite a bit"};
        slotVocab_["adjective"] = {"interesting", "cool", "fascinating", "great", "solid"};
        slotVocab_["emotion"] = {"good", "nice", "interesting", "curious", "thoughtful"};
        slotVocab_["action"] = {"works", "happens", "exists", "connects", "relates"};
        slotVocab_["reason"] = {"because it matters", "for good reason", "naturally", "logically"};
        slotVocab_["example"] = {"for instance", "like", "such as", "consider"};
        slotVocab_["connector"] = {"and", "also", "plus", "furthermore", "besides"};
        slotVocab_["opinion"] = {"I think", "seems like", "in my view", "honestly"};
        slotVocab_["greeting"] = {"hey", "hi there", "yo", "what's up", "hello"};
    }

    // Pick a random item from a vector
    std::string pickRandom(const std::vector<std::string>& options) {
        if (options.empty()) return "";
        std::uniform_int_distribution<size_t> dist(0, options.size() - 1);
        return options[dist(rng_)];
    }

    // Find the best slot filler based on context
    std::string fillSlot(const std::string& slotName, const MKCrystallizeContext& ctx) {
        // Priority 1: Use subject from context if slot is subject-like
        if ((slotName == "subject" || slotName == "topic" || slotName == "thing")
            && !ctx.subject.empty()) {
            return ctx.subject;
        }

        // Priority 2: Use emotion words if slot is emotion-like
        if ((slotName == "emotion" || slotName == "feeling" || slotName == "mood")
            && !ctx.emotion.empty()) {
            return ctx.emotion;
        }

        // Priority 3: Use facts from knowledge graph
        if ((slotName == "fact" || slotName == "info" || slotName == "detail")
            && !ctx.facts.empty()) {
            return pickRandom(ctx.facts);
        }

        // Priority 4: Use user's own vocab if available
        if (!ctx.userVocab.empty()) {
            // Check if any user vocab word is relevant to this slot type
            for (const auto& word : ctx.userVocab) {
                if (slotName == "adjective" && word.size() > 3) return word;
                if (slotName == "object" && word.size() > 2) return word;
            }
        }

        // Priority 5: Fall back to default vocab pool
        auto it = slotVocab_.find(slotName);
        if (it != slotVocab_.end() && !it->second.empty()) {
            return pickRandom(it->second);
        }

        // Last resort: use the slot name itself
        return slotName;
    }

    // Apply tone adjustments to the output
    std::string applyTone(const std::string& text, const std::string& tone) {
        if (tone == "casual") {
            // Make more casual: lowercase first char, add filler words sometimes
            std::string result = text;
            if (!result.empty() && std::isupper(result[0])) {
                result[0] = std::tolower(result[0]);
            }
            return result;
        } else if (tone == "formal") {
            // Capitalize first letter, ensure period at end
            std::string result = text;
            if (!result.empty() && std::islower(result[0])) {
                result[0] = std::toupper(result[0]);
            }
            if (!result.empty() && result.back() != '.' && result.back() != '!'
                && result.back() != '?') {
                result += '.';
            }
            return result;
        } else if (tone == "playful") {
            static const std::vector<std::string> suffixes = {
                " haha", " :)", " lol", " tbh", ""
            };
            std::uniform_int_distribution<size_t> dist(0, suffixes.size() - 1);
            return text + suffixes[dist(rng_)];
        }
        return text;
    }

public:
    MKCrystallizer() : rng_(std::random_device{}()) {
        initDefaultVocab();
    }

    // Fill a crystal's pattern slots with context-appropriate words
    std::string crystallize(const MKCrystal& crystal, const MKCrystallizeContext& ctx) {
        std::string result = crystal.pattern;

        // Replace each {slot} placeholder with appropriate content
        for (const auto& slot : crystal.slots) {
            std::string placeholder = "{" + slot + "}";
            std::string filler = fillSlot(slot, ctx);
            size_t pos = result.find(placeholder);
            if (pos != std::string::npos) {
                result.replace(pos, placeholder.size(), filler);
            }
        }

        // If pattern has no slots, use it directly (it's a fixed response)
        // Apply tone
        std::string tone = ctx.tone.empty() ? "casual" : ctx.tone;
        result = applyTone(result, tone);

        return result;
    }

    // Fuse multiple crystals into one combined response
    std::string fuse(const std::vector<const MKCrystal*>& crystals,
                     const MKCrystallizeContext& ctx) {
        if (crystals.empty()) return "";
        if (crystals.size() == 1) return crystallize(*crystals[0], ctx);

        // Take the primary crystal as base
        std::string primary = crystallize(*crystals[0], ctx);

        // Take elements from secondary crystals
        std::vector<std::string> additions;
        for (size_t i = 1; i < crystals.size() && i < 3; i++) {
            std::string secondary = crystallize(*crystals[i], ctx);
            // Use a connector between parts
            static const std::vector<std::string> connectors = {
                ". Also, ", ". And ", " -- ", ". Plus, ", ". "
            };
            std::uniform_int_distribution<size_t> dist(0, connectors.size() - 1);
            additions.push_back(connectors[dist(rng_)] + secondary);
        }

        std::string result = primary;
        for (const auto& add : additions) {
            result += add;
        }

        return result;
    }

    // Process feedback: increase or decrease crystal confidence
    void feedback(MKCrystalStore& store, int crystalId, bool positive) {
        MKCrystal* c = store.getMutableCrystal(crystalId);
        if (!c) return;

        if (positive) {
            c->confidence = std::min(1.0f, c->confidence + 0.05f);
            c->frequency = std::min(1.0f, c->frequency + 0.02f);
        } else {
            c->confidence = std::max(0.0f, c->confidence - 0.1f);
            c->frequency = std::max(0.0f, c->frequency - 0.01f);
        }
    }

    // Add vocabulary to a slot type
    void addVocab(const std::string& slotType, const std::string& word) {
        slotVocab_[slotType].push_back(word);
    }

    // Absorb user vocabulary for adaptive slot filling
    void absorbUserVocab(const std::string& input) {
        // Extract unique content words from user input and add to vocab pools
        std::string word;
        for (char c : input) {
            if (std::isalpha(c)) {
                word += std::tolower(c);
            } else {
                if (word.size() > 3) {
                    // Heuristic: long words are likely adjectives or objects
                    if (word.size() > 6) {
                        slotVocab_["adjective"].push_back(word);
                    } else {
                        slotVocab_["object"].push_back(word);
                    }
                }
                word.clear();
            }
        }
        if (word.size() > 3) {
            slotVocab_["object"].push_back(word);
        }
    }
};

#endif // MK_CXN_CRYSTALLIZER_CPP
