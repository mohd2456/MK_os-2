#ifndef MK_CXN_CRYSTAL_CPP
#define MK_CXN_CRYSTAL_CPP

#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include "../safe_parse.h"

// ===================================================================================
// MK CXN ENGINE - Crystal Data Structures & Store
// ===================================================================================
// A Crystal is a compressed language pattern: a template with slots, triggers,
// and metadata (domain, emotion, depth). The CrystalStore indexes them for O(1)
// trigger-word lookup and supports serialization to disk.
// ===================================================================================

struct MKCrystal {
    int id;
    int depth;                          // 0=surface, 1=simple, 2=compound, 3=complex, 4=philosophical
    std::vector<std::string> triggers;  // words that activate this crystal
    std::vector<std::string> slots;     // pattern slots: {subject}, {object}, {emotion}, etc.
    std::string pattern;                // template with {slot} placeholders
    std::string intent;                 // greeting, inform, question, comfort, explain, joke, farewell
    std::string domain;                 // general, tech, science, personal, philosophy, casual
    std::string emotion;                // neutral, happy, sad, curious, excited, calm, empathetic
    float frequency;                    // how often this crystal has been used (0.0 - 1.0)
    float confidence;                   // quality rating (0.0 - 1.0)

    MKCrystal()
        : id(0), depth(0), frequency(0.0f), confidence(0.5f) {}

    MKCrystal(int id_, int depth_, const std::vector<std::string>& triggers_,
              const std::string& pattern_, const std::string& intent_,
              const std::string& domain_, const std::string& emotion_,
              const std::vector<std::string>& slots_ = {})
        : id(id_), depth(depth_), triggers(triggers_), slots(slots_),
          pattern(pattern_), intent(intent_), domain(domain_), emotion(emotion_),
          frequency(0.0f), confidence(0.5f) {}

    std::string toString() const {
        std::ostringstream ss;
        ss << "Crystal[" << id << " d=" << depth << " '" << pattern << "']";
        return ss.str();
    }
};

// ===================================================================================
// MKCrystalStore - Stores crystals with O(1) trigger-word index
// ===================================================================================
class MKCrystalStore {
private:
    std::vector<MKCrystal> crystals_;
    std::unordered_map<std::string, std::vector<int>> triggerIndex_; // trigger word -> crystal IDs
    int nextId_;

    // Tokenize a string into lowercase words
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

    // Extract slot names from a pattern like "I think {subject} is {adjective}"
    static std::vector<std::string> extractSlots(const std::string& pattern) {
        std::vector<std::string> slots;
        size_t pos = 0;
        while (pos < pattern.size()) {
            size_t start = pattern.find('{', pos);
            if (start == std::string::npos) break;
            size_t end = pattern.find('}', start);
            if (end == std::string::npos) break;
            slots.push_back(pattern.substr(start + 1, end - start - 1));
            pos = end + 1;
        }
        return slots;
    }

    void indexCrystal(int id) {
        if (id < 0 || id >= (int)crystals_.size()) return;
        const auto& c = crystals_[id];
        for (const auto& trigger : c.triggers) {
            std::string lower = trigger;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            triggerIndex_[lower].push_back(id);
        }
    }

public:
    MKCrystalStore() : nextId_(0) {}

    // Add a pre-built crystal
    int addCrystal(const MKCrystal& crystal) {
        MKCrystal c = crystal;
        c.id = nextId_++;
        int id = c.id;
        crystals_.push_back(c);
        indexCrystal(id);
        return id;
    }

    // Create a crystal from a natural sentence (auto-extract triggers and slots)
    int createFromSentence(const std::string& sentence, const std::string& intent,
                           const std::string& domain, const std::string& emotion, int depth) {
        auto tokens = tokenize(sentence);
        // Use content words as triggers (skip stopwords)
        static const std::vector<std::string> stopwords = {
            "i", "me", "my", "we", "you", "your", "the", "a", "an", "is", "are",
            "was", "were", "be", "been", "being", "have", "has", "had", "do", "does",
            "did", "will", "would", "could", "should", "may", "might", "shall", "can",
            "to", "of", "in", "for", "on", "with", "at", "by", "from", "it", "its",
            "this", "that", "these", "those", "and", "or", "but", "if", "then", "so",
            "not", "no", "just", "very", "really", "also", "too", "than", "as"
        };
        std::vector<std::string> triggers;
        for (const auto& t : tokens) {
            if (t.size() > 2) {
                bool isStop = false;
                for (const auto& sw : stopwords) {
                    if (t == sw) { isStop = true; break; }
                }
                if (!isStop && triggers.size() < 6) {
                    triggers.push_back(t);
                }
            }
        }

        auto slots = extractSlots(sentence);
        MKCrystal c;
        c.id = nextId_++;
        c.depth = depth;
        c.triggers = triggers;
        c.slots = slots;
        c.pattern = sentence;
        c.intent = intent;
        c.domain = domain;
        c.emotion = emotion;
        c.frequency = 0.0f;
        c.confidence = 0.5f;

        int id = c.id;
        crystals_.push_back(c);
        indexCrystal(id);
        return id;
    }

    // Bootstrap store from arrays of text (casual responses, knowledge facts)
    void bootstrap(const std::vector<std::string>& casualTexts,
                   const std::vector<std::string>& knowledgeFacts) {
        // Create crystals from casual texts (depth 0-1, casual domain)
        for (const auto& text : casualTexts) {
            if (text.empty()) continue;
            int depth = (text.size() > 40) ? 1 : 0;
            std::string intent = "inform";
            if (text.find('?') != std::string::npos) intent = "question";
            else if (text.find('!') != std::string::npos) intent = "greeting";
            createFromSentence(text, intent, "casual", "neutral", depth);
        }
        // Create crystals from knowledge facts (depth 2-3, various domains)
        for (const auto& fact : knowledgeFacts) {
            if (fact.empty()) continue;
            std::string domain = "general";
            if (fact.find("python") != std::string::npos || fact.find("code") != std::string::npos)
                domain = "tech";
            else if (fact.find("atom") != std::string::npos || fact.find("energy") != std::string::npos)
                domain = "science";
            int depth = (fact.size() > 60) ? 3 : 2;
            createFromSentence(fact, "inform", domain, "neutral", depth);
        }
    }

    // Look up crystals by trigger word (O(1) via hash map)
    std::vector<int> lookupByTrigger(const std::string& word) const {
        std::string lower = word;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        auto it = triggerIndex_.find(lower);
        if (it != triggerIndex_.end()) return it->second;
        return {};
    }

    // Look up crystals by multiple triggers (union of all matches)
    std::vector<int> lookupByTriggers(const std::vector<std::string>& words) const {
        std::unordered_map<int, int> hitCount;
        for (const auto& w : words) {
            auto ids = lookupByTrigger(w);
            for (int id : ids) {
                hitCount[id]++;
            }
        }
        // Sort by hit count descending
        std::vector<std::pair<int, int>> sorted(hitCount.begin(), hitCount.end());
        std::sort(sorted.begin(), sorted.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });
        std::vector<int> result;
        result.reserve(sorted.size());
        for (const auto& p : sorted) result.push_back(p.first);
        return result;
    }

    // Get crystal by ID
    const MKCrystal* getCrystal(int id) const {
        if (id >= 0 && id < (int)crystals_.size()) return &crystals_[id];
        return nullptr;
    }

    // Get mutable crystal (for updating frequency/confidence)
    MKCrystal* getMutableCrystal(int id) {
        if (id >= 0 && id < (int)crystals_.size()) return &crystals_[id];
        return nullptr;
    }

    size_t size() const { return crystals_.size(); }

    // Save to disk (simple text format)
    void save(const std::string& filename) const {
        std::ofstream out(filename);
        if (!out.is_open()) return;
        out << crystals_.size() << "\n";
        for (const auto& c : crystals_) {
            out << c.id << "|" << c.depth << "|";
            // triggers joined by comma
            for (size_t i = 0; i < c.triggers.size(); i++) {
                out << c.triggers[i];
                if (i + 1 < c.triggers.size()) out << ",";
            }
            out << "|" << c.pattern << "|" << c.intent << "|" << c.domain
                << "|" << c.emotion << "|" << c.frequency << "|" << c.confidence << "\n";
        }
        out.close();
    }

    // Load from disk
    void load(const std::string& filename) {
        std::ifstream in(filename);
        if (!in.is_open()) return;
        std::string line;
        if (!std::getline(in, line)) return;
        // Parse defensively — a corrupt/partial state file must never crash boot.
        int count = mk::safeStoi(line, 0);
        crystals_.clear();
        triggerIndex_.clear();
        nextId_ = 0;
        for (int i = 0; i < count && std::getline(in, line); i++) {
            // Format: id|depth|triggers_csv|pattern|intent|domain|emotion|frequency|confidence
            std::vector<std::string> parts;
            std::istringstream ss(line);
            std::string part;
            while (std::getline(ss, part, '|')) parts.push_back(part);
            if (parts.size() < 9) continue;

            MKCrystal c;
            c.id = mk::safeStoi(parts[0], c.id);
            c.depth = mk::safeStoi(parts[1], c.depth);
            // Parse triggers
            std::istringstream trigSS(parts[2]);
            std::string trig;
            while (std::getline(trigSS, trig, ',')) {
                if (!trig.empty()) c.triggers.push_back(trig);
            }
            c.pattern = parts[3];
            c.intent = parts[4];
            c.domain = parts[5];
            c.emotion = parts[6];
            c.frequency = mk::safeStof(parts[7], c.frequency);
            c.confidence = mk::safeStof(parts[8], c.confidence);
            c.slots = extractSlots(c.pattern);

            if (c.id >= nextId_) nextId_ = c.id + 1;
            crystals_.push_back(c);
            indexCrystal(c.id);
        }
        in.close();
    }
};

#endif // MK_CXN_CRYSTAL_CPP
