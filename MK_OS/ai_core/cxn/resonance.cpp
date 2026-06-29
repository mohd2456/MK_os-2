#ifndef MK_CXN_RESONANCE_CPP
#define MK_CXN_RESONANCE_CPP

#include <string>
#include <vector>
#include <algorithm>
#include <unordered_set>
#include <cmath>

// ===================================================================================
// MK CXN ENGINE - Resonance Engine
// ===================================================================================
// Finds the best-matching crystals for a given input by scoring trigger overlap,
// domain match, emotion match, frequency, and confidence. Amplifies co-activating
// crystals and returns sorted results.
// ===================================================================================

#include "crystal.cpp"

struct MKResonanceResult {
    int crystalId;
    float score;           // combined resonance score (0.0 - 1.0+)
    float triggerOverlap;  // fraction of input words matching triggers
    float domainBoost;     // bonus for domain match
    float emotionBoost;    // bonus for emotion match
    float coActivation;    // amplification from related crystals firing

    MKResonanceResult()
        : crystalId(-1), score(0.0f), triggerOverlap(0.0f),
          domainBoost(0.0f), emotionBoost(0.0f), coActivation(0.0f) {}
};

class MKResonanceEngine {
private:
    // Scoring weights
    static constexpr float W_TRIGGER    = 0.40f;
    static constexpr float W_FREQUENCY  = 0.15f;
    static constexpr float W_CONFIDENCE = 0.15f;
    static constexpr float W_DOMAIN     = 0.15f;
    static constexpr float W_EMOTION    = 0.10f;
    static constexpr float W_COACT      = 0.05f;

    // Context state for domain/emotion matching
    std::string currentDomain_;
    std::string currentEmotion_;

    // Tokenize input to lowercase words
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

    // Detect domain from input keywords
    static std::string detectDomain(const std::vector<std::string>& tokens) {
        static const std::unordered_set<std::string> techWords = {
            "code", "python", "java", "program", "computer", "algorithm", "software",
            "debug", "compile", "function", "variable", "api", "server", "database"
        };
        static const std::unordered_set<std::string> scienceWords = {
            "atom", "molecule", "energy", "physics", "chemistry", "biology", "dna",
            "evolution", "quantum", "gravity", "cell", "experiment", "theory"
        };
        static const std::unordered_set<std::string> personalWords = {
            "feel", "feeling", "love", "hate", "friend", "family", "life", "dream",
            "hope", "fear", "happy", "sad", "angry", "worried", "excited"
        };
        static const std::unordered_set<std::string> philosophyWords = {
            "meaning", "exist", "consciousness", "truth", "reality", "morality",
            "purpose", "soul", "believe", "think", "universe", "infinite"
        };

        int tech = 0, science = 0, personal = 0, philosophy = 0;
        for (const auto& t : tokens) {
            if (techWords.count(t)) tech++;
            if (scienceWords.count(t)) science++;
            if (personalWords.count(t)) personal++;
            if (philosophyWords.count(t)) philosophy++;
        }
        int maxVal = std::max({tech, science, personal, philosophy});
        if (maxVal == 0) return "general";
        if (tech == maxVal) return "tech";
        if (science == maxVal) return "science";
        if (personal == maxVal) return "personal";
        return "philosophy";
    }

    // Detect emotion from input keywords
    static std::string detectEmotion(const std::vector<std::string>& tokens) {
        static const std::unordered_set<std::string> happyWords = {
            "happy", "great", "awesome", "love", "excited", "wonderful", "amazing", "good"
        };
        static const std::unordered_set<std::string> sadWords = {
            "sad", "depressed", "down", "lonely", "miss", "hurt", "cry", "lost"
        };
        static const std::unordered_set<std::string> curiousWords = {
            "what", "how", "why", "wonder", "curious", "explain", "tell", "know"
        };
        static const std::unordered_set<std::string> excitedWords = {
            "wow", "omg", "incredible", "insane", "mind", "blown", "epic", "fire"
        };

        int happy = 0, sad = 0, curious = 0, excited = 0;
        for (const auto& t : tokens) {
            if (happyWords.count(t)) happy++;
            if (sadWords.count(t)) sad++;
            if (curiousWords.count(t)) curious++;
            if (excitedWords.count(t)) excited++;
        }
        int maxVal = std::max({happy, sad, curious, excited});
        if (maxVal == 0) return "neutral";
        if (happy == maxVal) return "happy";
        if (sad == maxVal) return "sad";
        if (curious == maxVal) return "curious";
        return "excited";
    }

    // Score a single crystal against input tokens
    MKResonanceResult scoreCrystal(const MKCrystal& crystal,
                                    const std::vector<std::string>& inputTokens,
                                    const std::string& detectedDomain,
                                    const std::string& detectedEmotion) const {
        MKResonanceResult result;
        result.crystalId = crystal.id;

        // Trigger overlap: fraction of crystal triggers that match input tokens
        if (!crystal.triggers.empty()) {
            std::unordered_set<std::string> inputSet(inputTokens.begin(), inputTokens.end());
            int matches = 0;
            for (const auto& trigger : crystal.triggers) {
                std::string lower = trigger;
                std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                if (inputSet.count(lower)) matches++;
            }
            result.triggerOverlap = (float)matches / (float)crystal.triggers.size();
        }

        // Domain boost
        if (!detectedDomain.empty() && crystal.domain == detectedDomain) {
            result.domainBoost = 1.0f;
        } else if (crystal.domain == "general" || crystal.domain == "casual") {
            result.domainBoost = 0.3f; // general crystals always somewhat relevant
        }

        // Emotion boost
        if (!detectedEmotion.empty() && crystal.emotion == detectedEmotion) {
            result.emotionBoost = 1.0f;
        } else if (crystal.emotion == "neutral") {
            result.emotionBoost = 0.4f; // neutral crystals are safe defaults
        }

        // Combined score
        result.score = W_TRIGGER * result.triggerOverlap
                     + W_FREQUENCY * crystal.frequency
                     + W_CONFIDENCE * crystal.confidence
                     + W_DOMAIN * result.domainBoost
                     + W_EMOTION * result.emotionBoost;

        return result;
    }

public:
    MKResonanceEngine() : currentDomain_("general"), currentEmotion_("neutral") {}

    // Set context for subsequent resonance calls
    void setContext(const std::string& domain, const std::string& emotion) {
        currentDomain_ = domain;
        currentEmotion_ = emotion;
    }

    // Find best matching crystals for input
    std::vector<MKResonanceResult> resonate(const std::string& input,
                                             MKCrystalStore& store,
                                             size_t maxResults = 10) {
        auto tokens = tokenize(input);
        if (tokens.empty()) return {};

        // Detect context from input
        std::string domain = detectDomain(tokens);
        std::string emotion = detectEmotion(tokens);
        currentDomain_ = domain;
        currentEmotion_ = emotion;

        // Get candidate crystal IDs via trigger index (O(1) per word)
        auto candidateIds = store.lookupByTriggers(tokens);

        // If no trigger matches, use first N crystals as fallback
        if (candidateIds.empty()) {
            size_t fallbackCount = std::min((size_t)20, store.size());
            for (size_t i = 0; i < fallbackCount; i++) {
                candidateIds.push_back((int)i);
            }
        }

        // Cap candidates to avoid scoring too many
        if (candidateIds.size() > 100) candidateIds.resize(100);

        // Score each candidate
        std::vector<MKResonanceResult> results;
        results.reserve(candidateIds.size());
        for (int id : candidateIds) {
            const MKCrystal* c = store.getCrystal(id);
            if (!c) continue;
            auto res = scoreCrystal(*c, tokens, domain, emotion);
            if (res.score > 0.05f) { // skip near-zero scores
                results.push_back(res);
            }
        }

        // Co-activation: amplify crystals that share triggers with top scorer
        if (results.size() > 1) {
            std::sort(results.begin(), results.end(),
                      [](const auto& a, const auto& b) { return a.score > b.score; });
            const MKCrystal* topCrystal = store.getCrystal(results[0].crystalId);
            if (topCrystal) {
                std::unordered_set<std::string> topTriggers(
                    topCrystal->triggers.begin(), topCrystal->triggers.end());
                for (size_t i = 1; i < results.size(); i++) {
                    const MKCrystal* other = store.getCrystal(results[i].crystalId);
                    if (!other) continue;
                    int shared = 0;
                    for (const auto& t : other->triggers) {
                        if (topTriggers.count(t)) shared++;
                    }
                    if (shared > 0) {
                        float coAct = (float)shared / (float)topCrystal->triggers.size();
                        results[i].coActivation = coAct;
                        results[i].score += W_COACT * coAct;
                    }
                }
            }
        }

        // Final sort by score descending
        std::sort(results.begin(), results.end(),
                  [](const auto& a, const auto& b) { return a.score > b.score; });

        // Trim to max results
        if (results.size() > maxResults) results.resize(maxResults);
        return results;
    }

    // Get last detected domain/emotion
    std::string getDetectedDomain() const { return currentDomain_; }
    std::string getDetectedEmotion() const { return currentEmotion_; }
};

#endif // MK_CXN_RESONANCE_CPP
