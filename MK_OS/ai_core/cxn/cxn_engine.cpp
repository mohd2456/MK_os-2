#ifndef MK_CXN_ENGINE_CPP
#define MK_CXN_ENGINE_CPP

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <algorithm>

// ===================================================================================
// MK CXN ENGINE - Crystal Network (Master Class)
// ===================================================================================
// Combines CrystalStore + ResonanceEngine + Crystallizer into one generation API.
// Bootstrap with casual responses + knowledge facts + hand-crafted crystals.
// Main generate() method: extract triggers -> resonate -> select best -> crystallize.
// Designed as the primary response generator, with MCE and templates as fallbacks.
// ===================================================================================

#include "crystallizer.cpp"

class MKCrystalNetwork {
private:
    MKCrystalStore store_;
    MKResonanceEngine resonance_;
    MKCrystallizer crystallizer_;

    bool initialized_;
    std::string lastTrace_;
    int totalGenerations_;

    // Tokenize to lowercase
    static std::vector<std::string> tokenize(const std::string& text) {
        std::vector<std::string> tokens;
        std::string word;
        for (char c : text) {
            if (std::isalpha(c)) {
                word += std::tolower(c);
            } else {
                if (!word.empty()) {
                    tokens.push_back(word);
                    word.clear();
                }
            }
        }
        if (!word.empty()) tokens.push_back(word);
        return tokens;
    }

    // Add hand-crafted crystals covering depths 0-4
    void addHandCraftedCrystals() {
        // Depth 0: Surface greetings and simple reactions
        store_.addCrystal(MKCrystal(0, 0, {"hello", "hi", "hey"}, "hey! what's up?", "greeting", "casual", "happy"));
        store_.addCrystal(MKCrystal(0, 0, {"goodbye", "bye", "later"}, "catch you later!", "farewell", "casual", "happy"));
        store_.addCrystal(MKCrystal(0, 0, {"thanks", "thank"}, "no problem at all!", "greeting", "casual", "happy"));
        store_.addCrystal(MKCrystal(0, 0, {"good", "morning"}, "morning! hope you slept well", "greeting", "casual", "happy"));
        store_.addCrystal(MKCrystal(0, 0, {"good", "night"}, "night! rest well", "farewell", "casual", "calm"));
        store_.addCrystal(MKCrystal(0, 0, {"yes", "yeah", "yep"}, "got it!", "greeting", "casual", "neutral"));
        store_.addCrystal(MKCrystal(0, 0, {"no", "nope", "nah"}, "fair enough, no worries", "greeting", "casual", "neutral"));
        store_.addCrystal(MKCrystal(0, 0, {"okay", "alright", "sure"}, "cool, let's keep going then", "greeting", "casual", "neutral"));
        store_.addCrystal(MKCrystal(0, 0, {"wow", "amazing"}, "right?! pretty wild", "greeting", "casual", "excited"));
        store_.addCrystal(MKCrystal(0, 0, {"lol", "haha", "funny"}, "haha glad that hit right", "greeting", "casual", "happy"));

        // Depth 1: Simple informational responses
        store_.addCrystal(MKCrystal(0, 1, {"what", "is"}, "{subject} is {adjective} -- want me to dig deeper?", "inform", "general", "curious", {"subject", "adjective"}));
        store_.addCrystal(MKCrystal(0, 1, {"how", "does"}, "so basically, {subject} {action}", "inform", "general", "neutral", {"subject", "action"}));
        store_.addCrystal(MKCrystal(0, 1, {"why"}, "the reason is {reason}, at least from what I know", "inform", "general", "curious", {"reason"}));
        store_.addCrystal(MKCrystal(0, 1, {"who"}, "{subject} is known for being pretty impactful in that area", "inform", "general", "neutral", {"subject"}));
        store_.addCrystal(MKCrystal(0, 1, {"when"}, "that happened a while back -- {fact}", "inform", "general", "neutral", {"fact"}));
        store_.addCrystal(MKCrystal(0, 1, {"where"}, "{subject} is located somewhere I can look into for you", "inform", "general", "neutral", {"subject"}));
        store_.addCrystal(MKCrystal(0, 1, {"tell", "me"}, "sure! so {subject} -- {fact}", "inform", "general", "happy", {"subject", "fact"}));
        store_.addCrystal(MKCrystal(0, 1, {"know", "about"}, "yeah I know a bit about {subject}. {fact}", "inform", "general", "neutral", {"subject", "fact"}));
        store_.addCrystal(MKCrystal(0, 1, {"help"}, "I got you! what do you need help with exactly?", "question", "general", "empathetic"));
        store_.addCrystal(MKCrystal(0, 1, {"explain"}, "alright so {subject} works like this: {fact}", "explain", "general", "neutral", {"subject", "fact"}));

        // Depth 2: Compound responses with reasoning
        store_.addCrystal(MKCrystal(0, 2, {"python", "programming"}, "python is super versatile -- great for scripting, AI, web backends. {fact}", "inform", "tech", "happy", {"fact"}));
        store_.addCrystal(MKCrystal(0, 2, {"code", "bug", "error"}, "bugs happen to everyone. walk me through what you see and we can figure it out", "comfort", "tech", "empathetic"));
        store_.addCrystal(MKCrystal(0, 2, {"learn", "study"}, "learning {subject} takes time but the key is consistency. {reason}", "inform", "general", "calm", {"subject", "reason"}));
        store_.addCrystal(MKCrystal(0, 2, {"think", "opinion"}, "{opinion}, {subject} has real potential. {reason}", "inform", "general", "curious", {"opinion", "subject", "reason"}));
        store_.addCrystal(MKCrystal(0, 2, {"compare", "difference", "versus"}, "the main difference is scope -- {subject} handles {action} while the other takes a different path", "explain", "general", "neutral", {"subject", "action"}));
        store_.addCrystal(MKCrystal(0, 2, {"work", "job", "career"}, "career stuff can be stressful. what specifically are you dealing with?", "question", "personal", "empathetic"));
        store_.addCrystal(MKCrystal(0, 2, {"music", "song", "listen"}, "music is such a vibe. what kind of stuff are you into lately?", "question", "casual", "happy"));
        store_.addCrystal(MKCrystal(0, 2, {"food", "eat", "cook"}, "oh I love talking food. are you cooking something or looking for recs?", "question", "casual", "excited"));
        store_.addCrystal(MKCrystal(0, 2, {"game", "play", "gaming"}, "gaming is peak recreation. what are you playing these days?", "question", "casual", "excited"));
        store_.addCrystal(MKCrystal(0, 2, {"movie", "film", "watch"}, "nice! what kind of movies do you usually go for?", "question", "casual", "curious"));

        // Depth 3: Complex responses with multiple aspects
        store_.addCrystal(MKCrystal(0, 3, {"artificial", "intelligence", "ai"}, "AI is fascinating but also nuanced -- it learns patterns, not understanding. {fact}. the real question is where the line between tool and thinker is", "explain", "tech", "curious", {"fact"}));
        store_.addCrystal(MKCrystal(0, 3, {"universe", "space", "cosmos"}, "the scale of the universe is honestly humbling. {fact}. makes you wonder what else is out there", "inform", "science", "curious", {"fact"}));
        store_.addCrystal(MKCrystal(0, 3, {"brain", "mind", "neural"}, "the brain is wild -- billions of connections firing at once. {fact}. consciousness is still the biggest mystery", "inform", "science", "curious", {"fact"}));
        store_.addCrystal(MKCrystal(0, 3, {"history", "civilization"}, "history repeats in patterns. {fact}. the key is recognizing cycles before they complete", "inform", "general", "neutral", {"fact"}));
        store_.addCrystal(MKCrystal(0, 3, {"future", "tomorrow", "predict"}, "predicting the future is tricky but trends give clues. {fact}. personally I think the next decade will be wild", "inform", "general", "excited", {"fact"}));
        store_.addCrystal(MKCrystal(0, 3, {"relationship", "people"}, "relationships are complex systems -- communication and trust are the foundation. what's on your mind?", "question", "personal", "empathetic"));
        store_.addCrystal(MKCrystal(0, 3, {"stress", "anxiety", "overwhelm"}, "I hear you. being overwhelmed is valid. sometimes breaking things into small steps helps reclaim control", "comfort", "personal", "empathetic"));
        store_.addCrystal(MKCrystal(0, 3, {"creative", "art", "design"}, "creativity is about connecting dots others miss. {opinion}, the best work comes from constraints, not freedom", "inform", "general", "curious", {"opinion"}));
        store_.addCrystal(MKCrystal(0, 3, {"algorithm", "data", "structure"}, "algorithms are just recipes for computers. {fact}. the art is choosing which recipe fits the problem", "explain", "tech", "neutral", {"fact"}));
        store_.addCrystal(MKCrystal(0, 3, {"economy", "money", "market"}, "economics is part math, part psychology. {fact}. markets move on fear and greed more than logic", "inform", "general", "neutral", {"fact"}));

        // Depth 4: Philosophical / deep responses
        store_.addCrystal(MKCrystal(0, 4, {"meaning", "life", "purpose"}, "meaning is not found, it is built. every choice you make adds a brick to something only you can see. the question is not 'what is the meaning' but 'what meaning will you create'", "inform", "philosophy", "calm"));
        store_.addCrystal(MKCrystal(0, 4, {"consciousness", "aware", "self"}, "consciousness might be the universe looking at itself through biological lenses. we are matter that learned to contemplate matter", "inform", "philosophy", "curious"));
        store_.addCrystal(MKCrystal(0, 4, {"free", "will", "choice"}, "free will is a strange loop -- we feel free, yet our choices emerge from neurons we did not choose to have. maybe freedom is the illusion that keeps us sane", "inform", "philosophy", "curious"));
        store_.addCrystal(MKCrystal(0, 4, {"death", "mortality", "finite"}, "mortality gives weight to our moments. without an end, nothing would feel precious. the tick of the clock is what makes music beautiful", "inform", "philosophy", "calm"));
        store_.addCrystal(MKCrystal(0, 4, {"truth", "reality", "real"}, "truth is layered -- physical, logical, emotional, subjective. what is real depends on which layer you are standing on", "inform", "philosophy", "curious"));
        store_.addCrystal(MKCrystal(0, 4, {"time", "eternity"}, "time might not flow at all -- we just perceive it that way because memory only works in one direction. the present is all that truly exists", "inform", "philosophy", "calm"));
        store_.addCrystal(MKCrystal(0, 4, {"love", "connection"}, "love is the only force that defies entropy. everything else decays, but genuine connection builds complexity from chaos", "inform", "philosophy", "empathetic"));
        store_.addCrystal(MKCrystal(0, 4, {"knowledge", "wisdom"}, "knowledge is knowing that tomatoes are fruits. wisdom is not putting them in a fruit salad. the gap between the two is lived experience", "inform", "philosophy", "happy"));
        store_.addCrystal(MKCrystal(0, 4, {"infinite", "forever"}, "infinity breaks intuition. there are more numbers between 0 and 1 than there are integers. some infinities are larger than others", "inform", "science", "curious"));
        store_.addCrystal(MKCrystal(0, 4, {"exist", "nothing", "void"}, "the fact that anything exists at all is the deepest mystery. why is there something rather than nothing? maybe nothingness was unstable", "inform", "philosophy", "curious"));
    }

public:
    MKCrystalNetwork() : initialized_(false), totalGenerations_(0) {}

    // Initialize with bootstrap data
    void initialize(const std::vector<std::string>& casualTexts,
                    const std::vector<std::string>& knowledgeFacts) {
        // Bootstrap the crystal store from text arrays
        store_.bootstrap(casualTexts, knowledgeFacts);

        // Add 50 hand-crafted crystals at various depths
        addHandCraftedCrystals();

        initialized_ = true;
        std::cout << "[CXN:ENGINE] Crystal Network initialized. "
                  << "Crystals: " << store_.size() << " | "
                  << "Hand-crafted: 50 | "
                  << "Depths: 0-4\n";
    }

    // Main generation method: triggers -> resonate -> crystallize -> output
    std::string generate(const std::string& input) {
        if (!initialized_) return "";

        std::ostringstream trace;
        totalGenerations_++;

        // 1. Extract trigger words from input
        auto tokens = tokenize(input);
        trace << "Triggers: [";
        for (size_t i = 0; i < tokens.size() && i < 6; i++) {
            trace << tokens[i];
            if (i + 1 < tokens.size() && i + 1 < 6) trace << ", ";
        }
        trace << "]\n";

        // 2. Resonate: find best matching crystals
        auto results = resonance_.resonate(input, store_, 5);
        trace << "Resonance: " << results.size() << " matches";
        if (!results.empty()) {
            trace << " (top score: " << results[0].score << ")";
        }
        trace << "\n";

        if (results.empty()) {
            lastTrace_ = trace.str();
            return "";
        }

        // 3. Select best crystal (require minimum score threshold)
        const float MIN_SCORE = 0.15f;
        if (results[0].score < MIN_SCORE) {
            trace << "Score below threshold (" << MIN_SCORE << "), no output\n";
            lastTrace_ = trace.str();
            return "";
        }

        const MKCrystal* best = store_.getCrystal(results[0].crystalId);
        if (!best) {
            lastTrace_ = trace.str();
            return "";
        }
        trace << "Selected: " << best->toString() << "\n";

        // 4. Build crystallization context
        MKCrystallizeContext ctx;
        ctx.originalInput = input;
        ctx.emotion = resonance_.getDetectedEmotion();
        ctx.tone = "casual";

        // Extract subject from input (first noun-like content word)
        for (const auto& t : tokens) {
            if (t.size() > 3 && t != "what" && t != "does" && t != "this"
                && t != "that" && t != "about" && t != "tell") {
                ctx.subject = t;
                break;
            }
        }

        // Use crystal triggers as fact hints
        for (const auto& trigger : best->triggers) {
            ctx.facts.push_back(trigger + " is relevant here");
        }

        // 5. Crystallize: fill slots and generate output
        std::string output;
        if (results.size() >= 2 && results[1].score > MIN_SCORE * 1.5f) {
            // Fuse top 2 crystals if second is also strong
            const MKCrystal* second = store_.getCrystal(results[1].crystalId);
            if (second) {
                std::vector<const MKCrystal*> fuseCrystals = {best, second};
                output = crystallizer_.fuse(fuseCrystals, ctx);
                trace << "Fused with: " << second->toString() << "\n";
            } else {
                output = crystallizer_.crystallize(*best, ctx);
            }
        } else {
            output = crystallizer_.crystallize(*best, ctx);
        }

        trace << "Output: " << output << "\n";
        lastTrace_ = trace.str();

        // Boost frequency of used crystal
        MKCrystal* mutable_best = store_.getMutableCrystal(results[0].crystalId);
        if (mutable_best) {
            mutable_best->frequency = std::min(1.0f, mutable_best->frequency + 0.01f);
        }

        // Absorb user vocabulary for future slot filling
        crystallizer_.absorbUserVocab(input);

        return output;
    }

    // Provide positive/negative feedback on last generation
    void feedback(bool positive) {
        // Find the crystal used in last generation via trace
        // For simplicity, feedback the top crystal from last resonate
        // (In production you would store lastUsedCrystalId)
        (void)positive;
    }

    // Get the last thought trace
    std::string explain() const {
        if (lastTrace_.empty()) return "No CXN trace yet.";
        return lastTrace_;
    }

    // Stats
    std::string getStats() const {
        std::ostringstream ss;
        ss << "CXN Crystal Network Stats:\n"
           << "  Total crystals: " << store_.size() << "\n"
           << "  Total generations: " << totalGenerations_ << "\n"
           << "  Initialized: " << (initialized_ ? "yes" : "no") << "\n";
        return ss.str();
    }

    // Save/load crystal store
    void save(const std::string& filename) const {
        store_.save(filename);
    }

    void load(const std::string& filename) {
        store_.load(filename);
    }

    bool isInitialized() const { return initialized_; }
    size_t crystalCount() const { return store_.size(); }
};

#endif // MK_CXN_ENGINE_CPP
