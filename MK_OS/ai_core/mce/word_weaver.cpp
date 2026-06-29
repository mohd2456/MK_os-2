#ifndef MK_WORD_WEAVER_CPP
#define MK_WORD_WEAVER_CPP

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <random>
#include <chrono>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

// ===================================================================================
// MK CONSCIOUSNESS ENGINE — LAYER 4: WORD WEAVER
// ===================================================================================
// The actual sentence generator. Walks a word graph guided by the blueprint.
// Starts with ~5000 transitions from templates. Grows with every conversation.
// After a week of use: 50,000+ transitions = fluid unique sentences.
// ===================================================================================

class MKWordGraph {
private:
    // word → [(next_word, weight)]
    std::unordered_map<std::string, std::vector<std::pair<std::string, float>>> transitions_;
    // word → [tags like "casual", "sad", "tech"]
    std::unordered_map<std::string, std::vector<std::string>> word_tags_;

public:
    MKWordGraph() {}

    void addSequence(const std::vector<std::string>& words, const std::vector<std::string>& tags) {
        for (size_t i = 0; i + 1 < words.size(); i++) {
            auto& vec = transitions_[words[i]];
            // Check if transition already exists
            bool found = false;
            for (auto& p : vec) {
                if (p.first == words[i + 1]) {
                    p.second = std::min(5.0f, p.second + 0.5f); // reinforce
                    found = true;
                    break;
                }
            }
            if (!found) {
                vec.push_back({words[i + 1], 1.0f});
            }
            // Tag words
            for (const auto& tag : tags) {
                auto& wt = word_tags_[words[i]];
                bool hasTag = false;
                for (const auto& t : wt) { if (t == tag) { hasTag = true; break; } }
                if (!hasTag) wt.push_back(tag);
            }
        }
        // Tag last word too
        if (!words.empty()) {
            for (const auto& tag : tags) {
                auto& wt = word_tags_[words.back()];
                bool hasTag = false;
                for (const auto& t : wt) { if (t == tag) { hasTag = true; break; } }
                if (!hasTag) wt.push_back(tag);
            }
        }
    }

    void feedText(const std::string& text, const std::vector<std::string>& tags) {
        std::vector<std::string> words;
        std::istringstream iss(text);
        std::string word;
        while (iss >> word) {
            // Lowercase and strip trailing punctuation for graph
            std::string clean;
            for (char c : word) {
                if (std::isalpha(c) || c == '\'') clean += std::tolower(c);
            }
            if (!clean.empty()) words.push_back(clean);
        }
        if (words.size() >= 2) addSequence(words, tags);
    }

    std::string getNextWord(const std::string& currentWord,
                            const std::vector<std::string>& preferredTags,
                            const std::unordered_set<std::string>& avoidWords,
                            std::mt19937& rng) const {
        auto it = transitions_.find(currentWord);
        if (it == transitions_.end() || it->second.empty()) return "";

        const auto& candidates = it->second;
        // Score each candidate
        std::vector<std::pair<std::string, float>> scored;
        for (const auto& [word, weight] : candidates) {
            if (avoidWords.count(word)) continue;
            float score = weight;
            // Boost if word has preferred tag
            auto tagIt = word_tags_.find(word);
            if (tagIt != word_tags_.end()) {
                for (const auto& pt : preferredTags) {
                    for (const auto& t : tagIt->second) {
                        if (t == pt) { score *= 2.0f; break; }
                    }
                }
            }
            scored.push_back({word, score});
        }
        if (scored.empty()) return "";

        // Weighted random selection
        float total = 0;
        for (const auto& s : scored) total += s.second;
        std::uniform_real_distribution<float> dist(0.0f, total);
        float roll = dist(rng);
        float cumulative = 0;
        for (const auto& s : scored) {
            cumulative += s.second;
            if (roll <= cumulative) return s.first;
        }
        return scored.back().first;
    }

    size_t getSize() const { return transitions_.size(); }

    // Get all words that have a specific tag
    std::vector<std::string> getWordsByTag(const std::string& tag) const {
        std::vector<std::string> result;
        for (const auto& [word, tags] : word_tags_) {
            for (const auto& t : tags) {
                if (t == tag) { result.push_back(word); break; }
            }
        }
        return result;
    }

    // Get a random starting word with given tags
    std::string getRandomSeed(const std::vector<std::string>& preferredTags, std::mt19937& rng) const {
        auto candidates = getWordsByTag(preferredTags.empty() ? "casual" : preferredTags[0]);
        if (candidates.empty()) {
            // Fallback: any word that has transitions
            for (const auto& [w, _] : transitions_) {
                candidates.push_back(w);
                if (candidates.size() > 50) break;
            }
        }
        if (candidates.empty()) return "hey";
        std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
        return candidates[dist(rng)];
    }

    void save(const std::string& filename) const {
        std::ofstream out(filename);
        if (!out.is_open()) return;
        for (const auto& [word, nexts] : transitions_) {
            for (const auto& [next, weight] : nexts) {
                out << word << "|" << next << "|" << weight << "\n";
            }
        }
        out << "---TAGS---\n";
        for (const auto& [word, tags] : word_tags_) {
            out << word << "|";
            for (size_t i = 0; i < tags.size(); i++) {
                out << tags[i];
                if (i + 1 < tags.size()) out << ",";
            }
            out << "\n";
        }
        out.close();
    }

    void load(const std::string& filename) {
        std::ifstream in(filename);
        if (!in.is_open()) return;
        transitions_.clear();
        word_tags_.clear();
        std::string line;
        bool inTags = false;
        while (std::getline(in, line)) {
            if (line == "---TAGS---") { inTags = true; continue; }
            if (line.empty()) continue;
            if (!inTags) {
                std::istringstream ss(line);
                std::string w, n, ws;
                if (std::getline(ss, w, '|') && std::getline(ss, n, '|') && std::getline(ss, ws)) {
                    float weight = 1.0f;
                    try { weight = std::stof(ws); } catch (...) {}
                    transitions_[w].push_back({n, weight});
                }
            } else {
                size_t sep = line.find('|');
                if (sep != std::string::npos) {
                    std::string word = line.substr(0, sep);
                    std::string tagStr = line.substr(sep + 1);
                    std::istringstream ts(tagStr);
                    std::string tag;
                    while (std::getline(ts, tag, ',')) {
                        if (!tag.empty()) word_tags_[word].push_back(tag);
                    }
                }
            }
        }
        in.close();
    }
};

class MKWordWeaver {
private:
    MKWordGraph graph_;
    std::mt19937 rng_;

    // Connectors for when walk gets stuck
    std::vector<std::string> connectors_ = {"and", "but", "so", "like", "tho", "ngl", "fr"};

    // Intent seed words
    std::unordered_map<std::string, std::vector<std::string>> intentSeeds_ = {
        {"EMPATHIZE", {"damn", "that", "i", "nah", "bro"}},
        {"INFORM", {"so", "basically", "its", "that", "the"}},
        {"ENCOURAGE", {"you", "nah", "trust", "keep", "dont"}},
        {"ASK", {"what", "how", "you", "wait", "bro"}},
        {"JOKE", {"bro", "nah", "imagine", "you", "thats"}},
        {"AGREE", {"fr", "facts", "real", "yeah", "exactly"}},
        {"DISAGREE", {"nah", "i", "thats", "not", "eh"}},
        {"GREET", {"yo", "hey", "ayy", "sup", "whats"}},
        {"BYE", {"peace", "later", "aight", "catch", "see"}},
        {"OPINION", {"honestly", "ngl", "i", "thats", "lowkey"}}
    };

    std::string intentName(MKThoughtBlueprint::Intent i) const {
        static const char* names[] = {"EMPATHIZE","INFORM","ENCOURAGE","ASK","JOKE","AGREE","DISAGREE","GREET","BYE","OPINION"};
        return names[(int)i];
    }

    std::string capitalize(const std::string& s) const {
        if (s.empty()) return s;
        std::string r = s;
        r[0] = std::toupper(r[0]);
        return r;
    }

public:
    MKWordWeaver() {
        auto seed = std::chrono::steady_clock::now().time_since_epoch().count();
        rng_ = std::mt19937(static_cast<unsigned>(seed));
        std::cout << "[MCE:WORD_WEAVER] initialized\n";
    }

    // Bootstrap word graph with casual responses and knowledge
    void bootstrap(const std::vector<std::string>& casualResponses,
                   const std::vector<std::string>& knowledgeFacts) {
        // Feed all casual responses
        for (const auto& resp : casualResponses) {
            graph_.feedText(resp, {"casual", "friend"});
        }
        // Feed knowledge facts
        for (const auto& fact : knowledgeFacts) {
            graph_.feedText(fact, {"fact", "knowledge"});
        }
    }

    // Feed new text into the word graph (grows over time)
    void feed(const std::string& text, const std::vector<std::string>& tags) {
        graph_.feedText(text, tags);
    }

    // Main weaving method
    std::string weave(const MKThoughtBlueprint& blueprint) {
        std::string iName = intentName(blueprint.primaryIntent);
        std::vector<std::string> preferredTags = {blueprint.targetTone, "casual"};

        // 1. Pick seed word
        std::string current;
        auto seedIt = intentSeeds_.find(iName);
        if (seedIt != intentSeeds_.end() && !seedIt->second.empty()) {
            std::uniform_int_distribution<size_t> dist(0, seedIt->second.size() - 1);
            current = seedIt->second[dist(rng_)];
        }
        if (current.empty() || graph_.getSize() == 0) {
            current = graph_.getRandomSeed(preferredTags, rng_);
        }

        // 2. Walk the word graph
        std::string result = current;
        std::unordered_set<std::string> used;
        used.insert(current);
        int wordCount = 1;
        int stuckCount = 0;
        bool factInjected = false;

        while (wordCount < blueprint.targetLength && stuckCount < 3) {
            std::string next = graph_.getNextWord(current, preferredTags, used, rng_);

            if (next.empty()) {
                // Stuck — use connector and restart
                if (stuckCount < 2) {
                    std::uniform_int_distribution<size_t> cd(0, connectors_.size() - 1);
                    std::string conn = connectors_[cd(rng_)];
                    result += " " + conn;
                    wordCount++;
                    // Get a new seed related to subject or intent
                    current = graph_.getRandomSeed(preferredTags, rng_);
                    result += " " + current;
                    wordCount++;
                    used.clear(); // reset used to allow revisiting
                    used.insert(current);
                }
                stuckCount++;
                continue;
            }

            result += " " + next;
            used.insert(next);
            current = next;
            wordCount++;

            // Inject a fact at a natural midpoint if available
            if (!factInjected && !blueprint.factsToInclude.empty() &&
                wordCount >= blueprint.targetLength / 2) {
                std::uniform_int_distribution<size_t> fd(0, blueprint.factsToInclude.size() - 1);
                std::string fact = blueprint.factsToInclude[fd(rng_)];
                // Try to find the fact word in graph
                std::string factNext = graph_.getNextWord(fact, preferredTags, used, rng_);
                if (!factNext.empty()) {
                    result += " " + fact + " " + factNext;
                    wordCount += 2;
                    current = factNext;
                }
                factInjected = true;
            }
        }

        // 3. Grammar smoothing
        if (!result.empty()) {
            // Capitalize first letter
            result[0] = std::toupper(result[0]);
        }

        return result;
    }

    MKWordGraph& getGraph() { return graph_; }
    size_t getGraphSize() const { return graph_.getSize(); }

    void save(const std::string& filename) const { graph_.save(filename); }
    void load(const std::string& filename) { graph_.load(filename); }
};

#endif // MK_WORD_WEAVER_CPP
