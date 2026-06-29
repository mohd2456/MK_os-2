#ifndef MK_THOUGHT_ATOMS_CPP
#define MK_THOUGHT_ATOMS_CPP

#include <string>
#include <vector>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <sstream>

// ===================================================================================
// MK CONSCIOUSNESS ENGINE — LAYER 1: THOUGHT ATOMS
// ===================================================================================
// The smallest unit of meaning. Everything gets decomposed into atoms.
// A sentence like "I'm stressed about my physics exam" becomes:
//   [ENTITY:physics exam] [EMOTION:stressed] [STATE:worried] [ACTION:vent]
// Pure keyword-based decomposition. No NLP. Runs on anything.
// ===================================================================================

enum class MKAtomType {
    ENTITY,      // Nouns, proper nouns — "things"
    RELATION,    // Verbs, prepositions connecting entities
    EMOTION,     // Detected emotional state
    STATE,       // Current state of something
    PROPERTY,    // Adjective/descriptor
    ACTION,      // What user wants (ask, tell, learn, vent, request)
    SELF,        // Self-referential ("I", "me", "my")
    EVENT,       // Something that happened
    TONE,        // casual, formal, serious, playful
    CERTAINTY    // definite vs uncertain
};

struct MKThoughtAtom {
    MKAtomType type;
    std::string value;
    float weight;       // 0.0 - 1.0 importance
    long long timestamp;
    std::string source; // where it came from (user input, memory, graph)

    MKThoughtAtom() : type(MKAtomType::ENTITY), value(""), weight(0.5f), timestamp(0), source("") {}
    MKThoughtAtom(MKAtomType t, const std::string& v, float w, const std::string& src = "input")
        : type(t), value(v), weight(w), source(src) {
        timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
    }

    std::string toString() const {
        static const char* typeNames[] = {
            "ENTITY", "RELATION", "EMOTION", "STATE", "PROPERTY",
            "ACTION", "SELF", "EVENT", "TONE", "CERTAINTY"
        };
        int idx = static_cast<int>(type);
        std::string tName = (idx >= 0 && idx < 10) ? typeNames[idx] : "?";
        return "[" + tName + ":" + value + " w=" + std::to_string(weight).substr(0, 4) + "]";
    }
};

class MKAtomDecomposer {
private:
    // Emotion keywords (mirrors conversation_mode.cpp)
    std::vector<std::string> emo_happy_ = {"happy", "hyped", "excited", "amazing", "great", "awesome", "fire", "sick", "lol", "lmao", "lets go"};
    std::vector<std::string> emo_sad_ = {"sad", "depressed", "tired", "exhausted", "failed", "sucks", "hate", "crying", "lonely", "hopeless", "drained"};
    std::vector<std::string> emo_angry_ = {"angry", "pissed", "annoyed", "frustrated", "furious", "mad", "rage", "livid", "fed up"};
    std::vector<std::string> emo_nervous_ = {"nervous", "scared", "worried", "anxious", "stressed", "panicking", "overthinking", "terrified", "dreading"};
    std::vector<std::string> emo_chill_ = {"bored", "whatever", "idk", "meh", "chillin", "vibing"};

    // Tone indicators
    std::vector<std::string> tone_casual_ = {"bro", "dude", "lol", "ngl", "fr", "bruh", "yo", "tbh", "lowkey", "highkey", "ong"};
    std::vector<std::string> tone_formal_ = {"therefore", "furthermore", "regarding", "concerning", "additionally", "however"};
    std::vector<std::string> tone_serious_ = {"need", "must", "important", "urgent", "critical", "help", "please"};
    std::vector<std::string> tone_playful_ = {"haha", "lmao", "lol", "xd", "hehe", "omg", "yooo", "sheesh"};

    // Certainty markers
    std::vector<std::string> cert_definite_ = {"is", "always", "never", "definitely", "obviously", "clearly", "absolutely", "100%"};
    std::vector<std::string> cert_uncertain_ = {"maybe", "think", "might", "probably", "possibly", "idk", "not sure", "kinda", "perhaps"};

    // Action markers
    std::vector<std::string> act_ask_ = {"what", "how", "why", "when", "where", "who", "which", "?"};
    std::vector<std::string> act_tell_ = {"so", "basically", "the thing is", "guess what", "listen"};
    std::vector<std::string> act_vent_ = {"ugh", "man", "dude", "i hate", "so annoyed", "cant believe"};
    std::vector<std::string> act_request_ = {"can you", "help me", "i need", "show me", "tell me"};

    // Relation words (verbs/prepositions)
    std::vector<std::string> relations_ = {"is", "are", "was", "has", "have", "can", "will", "does", "did", "made", "got", "went", "said", "about", "with", "from", "for", "into", "like"};

    // Self-reference words
    std::vector<std::string> self_words_ = {"i", "me", "my", "mine", "myself", "im", "i'm", "ive", "i've"};

    // Stop words to skip for entity detection
    std::vector<std::string> stop_words_ = {"the", "a", "an", "is", "are", "was", "were", "be", "been", "to", "of", "and", "or", "but", "in", "on", "at", "it", "that", "this", "so", "just", "very", "really"};

    std::string toLower(const std::string& s) const {
        std::string r = s;
        std::transform(r.begin(), r.end(), r.begin(), ::tolower);
        return r;
    }

    bool contains(const std::string& text, const std::vector<std::string>& words) const {
        for (const auto& w : words) {
            if (text.find(w) != std::string::npos) return true;
        }
        return false;
    }

    std::string findFirst(const std::string& text, const std::vector<std::string>& words) const {
        for (const auto& w : words) {
            if (text.find(w) != std::string::npos) return w;
        }
        return "";
    }

    std::vector<std::string> splitWords(const std::string& text) const {
        std::vector<std::string> words;
        std::istringstream iss(text);
        std::string word;
        while (iss >> word) {
            // Strip basic punctuation from edges
            while (!word.empty() && (word.back() == '.' || word.back() == ',' || word.back() == '!' || word.back() == '?'))
                word.pop_back();
            if (!word.empty()) words.push_back(word);
        }
        return words;
    }

    bool isStopWord(const std::string& w) const {
        for (const auto& sw : stop_words_) {
            if (w == sw) return true;
        }
        return false;
    }

    bool isRelation(const std::string& w) const {
        for (const auto& r : relations_) {
            if (w == r) return true;
        }
        return false;
    }

    bool isSelf(const std::string& w) const {
        for (const auto& s : self_words_) {
            if (w == s) return true;
        }
        return false;
    }

public:
    MKAtomDecomposer() {
        std::cout << "[MCE:THOUGHT_ATOMS] initialized\n";
    }

    std::vector<MKThoughtAtom> decompose(const std::string& input) {
        std::vector<MKThoughtAtom> atoms;
        std::string lower = toLower(input);
        std::vector<std::string> words = splitWords(lower);

        // 1. Detect emotions
        std::string emo;
        if (!(emo = findFirst(lower, emo_happy_)).empty()) atoms.emplace_back(MKAtomType::EMOTION, "happy", 0.8f);
        if (!(emo = findFirst(lower, emo_sad_)).empty()) atoms.emplace_back(MKAtomType::EMOTION, "sad", 0.8f);
        if (!(emo = findFirst(lower, emo_angry_)).empty()) atoms.emplace_back(MKAtomType::EMOTION, "angry", 0.8f);
        if (!(emo = findFirst(lower, emo_nervous_)).empty()) atoms.emplace_back(MKAtomType::EMOTION, "nervous", 0.8f);
        if (!(emo = findFirst(lower, emo_chill_)).empty()) atoms.emplace_back(MKAtomType::EMOTION, "chill", 0.6f);

        // 2. Detect tone
        if (contains(lower, tone_casual_)) atoms.emplace_back(MKAtomType::TONE, "casual", 0.7f);
        else if (contains(lower, tone_formal_)) atoms.emplace_back(MKAtomType::TONE, "formal", 0.7f);
        else if (contains(lower, tone_serious_)) atoms.emplace_back(MKAtomType::TONE, "serious", 0.7f);
        else if (contains(lower, tone_playful_)) atoms.emplace_back(MKAtomType::TONE, "playful", 0.7f);
        else atoms.emplace_back(MKAtomType::TONE, "neutral", 0.3f);

        // 3. Detect certainty
        if (contains(lower, cert_definite_)) atoms.emplace_back(MKAtomType::CERTAINTY, "definite", 0.7f);
        else if (contains(lower, cert_uncertain_)) atoms.emplace_back(MKAtomType::CERTAINTY, "uncertain", 0.7f);

        // 4. Detect actions
        if (contains(lower, act_ask_)) atoms.emplace_back(MKAtomType::ACTION, "ask", 0.7f);
        if (contains(lower, act_tell_)) atoms.emplace_back(MKAtomType::ACTION, "tell", 0.6f);
        if (contains(lower, act_vent_)) atoms.emplace_back(MKAtomType::ACTION, "vent", 0.7f);
        if (contains(lower, act_request_)) atoms.emplace_back(MKAtomType::ACTION, "request", 0.8f);

        // 5. Detect entities, relations, self-reference from words
        for (const auto& w : words) {
            if (isSelf(w)) {
                atoms.emplace_back(MKAtomType::SELF, w, 0.5f);
            } else if (isRelation(w)) {
                atoms.emplace_back(MKAtomType::RELATION, w, 0.4f);
            } else if (!isStopWord(w) && w.length() > 2) {
                // Anything remaining that's not a stop word is likely an entity
                atoms.emplace_back(MKAtomType::ENTITY, w, 0.6f);
            }
        }

        return atoms;
    }
};

#endif // MK_THOUGHT_ATOMS_CPP
