#ifndef MK_ECHO_MEMORY_CPP
#define MK_ECHO_MEMORY_CPP

#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

// ===================================================================================
// MK CONSCIOUSNESS ENGINE — LAYER 5: ECHO MEMORY
// ===================================================================================
// Makes MK become YOU over time. Absorbs user's speech patterns, vocabulary,
// preferred response length, tone, and slang. After 100+ exchanges MK starts
// mirroring your communication style.
// ===================================================================================

struct MKUserProfile {
    std::unordered_map<std::string, int> wordFrequency;  // user's most used words
    std::string preferredTone;   // casual, formal, chill, hype
    int preferredLength;         // average word count user likes
    int totalMessages;
    int positiveReactions;       // user continued = positive
    int negativeReactions;       // user changed topic = negative
    std::vector<std::string> slangWords;  // user's slang/shortcuts
    float energyLevel;           // 0=low energy, 1=high energy

    MKUserProfile() : preferredTone("casual"), preferredLength(10),
                      totalMessages(0), positiveReactions(0),
                      negativeReactions(0), energyLevel(0.5f) {}
};

class MKEchoMemory {
private:
    MKUserProfile profile_;
    std::string lastUserInput_;
    std::string lastMkResponse_;
    MKWordGraph* wordGraph_;  // reference to word weaver's graph

    // Common slang detection
    std::vector<std::string> slangDetectors_ = {
        "bruh", "fr", "ngl", "ong", "lowkey", "highkey", "no cap",
        "bussin", "finna", "bet", "sus", "vibe", "slay", "goated",
        "mid", "w", "l", "deadass", "tbh", "imo", "rn", "wya",
        "smh", "icl", "istg", "iykyk", "periodt", "sheesh"
    };

    // Count words in a string
    int wordCount(const std::string& s) const {
        int c = 0; bool inW = false;
        for (char ch : s) {
            if (ch == ' ') inW = false;
            else if (!inW) { inW = true; c++; }
        }
        return c;
    }

    // Detect energy level from text
    float detectEnergy(const std::string& text) const {
        std::string lower = text;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        float energy = 0.5f;
        // High energy indicators
        if (lower.find("!!") != std::string::npos) energy += 0.2f;
        if (lower.find("lmao") != std::string::npos) energy += 0.15f;
        if (lower.find("yooo") != std::string::npos) energy += 0.15f;
        if (lower.find("lets go") != std::string::npos) energy += 0.2f;
        if (lower.find("omg") != std::string::npos) energy += 0.1f;
        // Low energy indicators
        if (lower.find("meh") != std::string::npos) energy -= 0.2f;
        if (lower.find("whatever") != std::string::npos) energy -= 0.15f;
        if (lower.find("idk") != std::string::npos) energy -= 0.1f;
        if (lower.find("tired") != std::string::npos) energy -= 0.2f;
        return std::max(0.0f, std::min(1.0f, energy));
    }

    // Extract individual words from text
    std::vector<std::string> extractWords(const std::string& text) const {
        std::vector<std::string> words;
        std::istringstream iss(text);
        std::string word;
        while (iss >> word) {
            std::string clean;
            for (char c : word) {
                if (std::isalpha(c) || c == '\'') clean += std::tolower(c);
            }
            if (!clean.empty() && clean.length() > 1) words.push_back(clean);
        }
        return words;
    }

public:
    MKEchoMemory() : wordGraph_(nullptr) {
        std::cout << "[MCE:ECHO_MEMORY] initialized\n";
    }

    void setWordGraph(MKWordGraph* graph) { wordGraph_ = graph; }

    // Absorb a user exchange to learn patterns
    void absorb(const std::string& userInput, const std::string& mkResponse, bool userContinued) {
        profile_.totalMessages++;
        lastUserInput_ = userInput;
        lastMkResponse_ = mkResponse;

        // Track user reaction
        if (userContinued) {
            profile_.positiveReactions++;
        } else {
            profile_.negativeReactions++;
        }

        // Extract and track word frequency
        auto words = extractWords(userInput);
        for (const auto& w : words) {
            profile_.wordFrequency[w]++;
        }

        // Feed user's word pairs into word graph (learn vocabulary)
        if (wordGraph_ && words.size() >= 2) {
            wordGraph_->feedText(userInput, {"user", "echo"});
        }

        // Detect and track slang usage
        std::string lower = userInput;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        for (const auto& slang : slangDetectors_) {
            if (lower.find(slang) != std::string::npos) {
                bool alreadyKnown = false;
                for (const auto& s : profile_.slangWords) {
                    if (s == slang) { alreadyKnown = true; break; }
                }
                if (!alreadyKnown) profile_.slangWords.push_back(slang);
            }
        }

        // Track preferred length (rolling average)
        int len = wordCount(userInput);
        profile_.preferredLength = (profile_.preferredLength * (profile_.totalMessages - 1) + len) / profile_.totalMessages;

        // Track energy level (rolling average)
        float energy = detectEnergy(userInput);
        profile_.energyLevel = profile_.energyLevel * 0.8f + energy * 0.2f;

        // Detect tone preference
        int casualCount = 0, formalCount = 0;
        for (const auto& slang : slangDetectors_) {
            if (lower.find(slang) != std::string::npos) casualCount++;
        }
        static const std::vector<std::string> formalWords = {"therefore", "however", "regarding", "furthermore"};
        for (const auto& fw : formalWords) {
            if (lower.find(fw) != std::string::npos) formalCount++;
        }
        if (casualCount > formalCount + 1) profile_.preferredTone = "casual";
        else if (formalCount > casualCount) profile_.preferredTone = "formal";
    }

    // Get user's most common words
    std::vector<std::string> getUserVocabulary(int topN = 20) const {
        std::vector<std::pair<std::string, int>> sorted(profile_.wordFrequency.begin(), profile_.wordFrequency.end());
        std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) {
            return a.second > b.second;
        });
        std::vector<std::string> result;
        for (int i = 0; i < topN && i < (int)sorted.size(); i++) {
            result.push_back(sorted[i].first);
        }
        return result;
    }

    std::string getPreferredTone() const { return profile_.preferredTone; }
    int getPreferredLength() const { return profile_.preferredLength; }

    // Adjust a blueprint to match user preferences
    void adjustBlueprint(MKThoughtBlueprint& blueprint) const {
        // Match tone
        if (!profile_.preferredTone.empty()) {
            blueprint.targetTone = profile_.preferredTone;
        }

        // Adjust length — match user's preferred length (but cap at 30)
        if (profile_.totalMessages > 10) {
            int adjusted = (blueprint.targetLength + profile_.preferredLength) / 2;
            blueprint.targetLength = std::max(5, std::min(30, adjusted));
        }

        // If user uses short messages, shorten response
        if (profile_.preferredLength <= 5 && profile_.totalMessages > 20) {
            blueprint.targetLength = std::min(blueprint.targetLength, 8);
        }

        // Boost energy if user is high energy
        if (profile_.energyLevel > 0.7f) {
            blueprint.targetTone = "hype";
        } else if (profile_.energyLevel < 0.3f) {
            blueprint.targetTone = "chill";
        }
    }

    // Get a summary of the user profile
    std::string getProfile() const {
        std::ostringstream ss;
        ss << "Echo Profile (" << profile_.totalMessages << " messages absorbed)\n";
        ss << "  Tone: " << profile_.preferredTone << "\n";
        ss << "  Avg length: " << profile_.preferredLength << " words\n";
        ss << "  Energy: " << (int)(profile_.energyLevel * 100) << "%\n";
        ss << "  Vocab size: " << profile_.wordFrequency.size() << " unique words\n";
        ss << "  Slang detected: ";
        for (size_t i = 0; i < profile_.slangWords.size() && i < 10; i++) {
            ss << profile_.slangWords[i];
            if (i + 1 < profile_.slangWords.size() && i + 1 < 10) ss << ", ";
        }
        ss << "\n  Positive reactions: " << profile_.positiveReactions
           << " | Negative: " << profile_.negativeReactions << "\n";
        auto topWords = getUserVocabulary(10);
        ss << "  Top words: ";
        for (size_t i = 0; i < topWords.size(); i++) {
            ss << topWords[i];
            if (i + 1 < topWords.size()) ss << ", ";
        }
        return ss.str();
    }

    void save(const std::string& filename) const {
        std::ofstream out(filename);
        if (!out.is_open()) return;
        out << "TONE|" << profile_.preferredTone << "\n";
        out << "LENGTH|" << profile_.preferredLength << "\n";
        out << "MESSAGES|" << profile_.totalMessages << "\n";
        out << "POSITIVE|" << profile_.positiveReactions << "\n";
        out << "NEGATIVE|" << profile_.negativeReactions << "\n";
        out << "ENERGY|" << profile_.energyLevel << "\n";
        out << "---SLANG---\n";
        for (const auto& s : profile_.slangWords) out << s << "\n";
        out << "---VOCAB---\n";
        for (const auto& [word, count] : profile_.wordFrequency) {
            out << word << "|" << count << "\n";
        }
        out.close();
    }

    void load(const std::string& filename) {
        std::ifstream in(filename);
        if (!in.is_open()) return;
        profile_ = MKUserProfile(); // reset
        std::string line;
        int section = 0; // 0=header, 1=slang, 2=vocab
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            if (line == "---SLANG---") { section = 1; continue; }
            if (line == "---VOCAB---") { section = 2; continue; }
            if (section == 0) {
                size_t sep = line.find('|');
                if (sep == std::string::npos) continue;
                std::string key = line.substr(0, sep);
                std::string val = line.substr(sep + 1);
                if (key == "TONE") profile_.preferredTone = val;
                else if (key == "LENGTH") profile_.preferredLength = std::stoi(val);
                else if (key == "MESSAGES") profile_.totalMessages = std::stoi(val);
                else if (key == "POSITIVE") profile_.positiveReactions = std::stoi(val);
                else if (key == "NEGATIVE") profile_.negativeReactions = std::stoi(val);
                else if (key == "ENERGY") profile_.energyLevel = std::stof(val);
            } else if (section == 1) {
                profile_.slangWords.push_back(line);
            } else if (section == 2) {
                size_t sep = line.find('|');
                if (sep != std::string::npos) {
                    profile_.wordFrequency[line.substr(0, sep)] = std::stoi(line.substr(sep + 1));
                }
            }
        }
        in.close();
    }

    int getTotalMessages() const { return profile_.totalMessages; }
    size_t getVocabSize() const { return profile_.wordFrequency.size(); }
};

#endif // MK_ECHO_MEMORY_CPP
