#ifndef MK_PERSISTENT_MEMORY_CPP
#define MK_PERSISTENT_MEMORY_CPP

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <ctime>
#include <algorithm>
#include <unordered_map>

// ===================================================================================
// MK PERSISTENT MEMORY
// Stores user interactions, preferences, and Q&A history across sessions.
// Uses a simple flat-file format for maximum portability (no SQLite dependency).
// Features:
//   - Timestamped interaction log
//   - User preference tracking (learned over time)
//   - Previous Q&A recall for context continuity
//   - Automatic load on boot, save on shutdown
//   - Configurable memory limits with oldest-first pruning
//   - Efficient memory footprint for low-end hardware
// File format:
//   [INTERACTION] timestamp|type|content
//   [PREFERENCE] key|value|confidence|last_updated
//   [QA] timestamp|question|answer|confidence
// ===================================================================================

struct MKInteraction {
    std::time_t timestamp;
    std::string type;       // "query", "command", "feedback", "correction"
    std::string content;
};

struct MKPreference {
    std::string key;
    std::string value;
    float confidence;       // 0.0-1.0, increases with repeated observations
    std::time_t lastUpdated;
};

struct MKQAEntry {
    std::time_t timestamp;
    std::string question;
    std::string answer;
    float confidence;
};

class MKPersistentMemory {
private:
    std::vector<MKInteraction> interactions;
    std::unordered_map<std::string, MKPreference> preferences;
    std::vector<MKQAEntry> qaHistory;

    std::string memoryFilePath;
    int maxInteractions;
    int maxQAEntries;
    bool dirty;             // Has unsaved changes

    // Format a timestamp as ISO string
    std::string formatTime(std::time_t t) {
        char buf[64];
        struct tm* tm_info = std::localtime(&t);
        if (tm_info) {
            std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", tm_info);
            return std::string(buf);
        }
        return std::to_string(t);
    }

    // Parse ISO timestamp back to time_t
    std::time_t parseTime(const std::string& timeStr) {
        // Try to parse as integer first (epoch seconds)
        try {
            return (std::time_t)std::stol(timeStr);
        } catch (...) {}
        // Fallback: return current time
        return std::time(nullptr);
    }

    // Escape pipe characters in content for safe storage
    std::string escapePipe(const std::string& s) {
        std::string result;
        for (char c : s) {
            if (c == '|') result += "\\|";
            else if (c == '\n') result += "\\n";
            else if (c == '\\') result += "\\\\";
            else result += c;
        }
        return result;
    }

    // Unescape pipe characters when loading
    std::string unescapePipe(const std::string& s) {
        std::string result;
        for (size_t i = 0; i < s.size(); i++) {
            if (s[i] == '\\' && i + 1 < s.size()) {
                if (s[i+1] == '|') { result += '|'; i++; }
                else if (s[i+1] == 'n') { result += '\n'; i++; }
                else if (s[i+1] == '\\') { result += '\\'; i++; }
                else result += s[i];
            } else {
                result += s[i];
            }
        }
        return result;
    }

    // Split a line by unescaped pipe delimiter
    std::vector<std::string> splitByPipe(const std::string& line) {
        std::vector<std::string> parts;
        std::string current;
        for (size_t i = 0; i < line.size(); i++) {
            if (line[i] == '\\' && i + 1 < line.size()) {
                current += line[i];
                current += line[i+1];
                i++;
            } else if (line[i] == '|') {
                parts.push_back(current);
                current.clear();
            } else {
                current += line[i];
            }
        }
        parts.push_back(current);
        return parts;
    }

    // Prune oldest entries if over limit
    void pruneIfNeeded() {
        if ((int)interactions.size() > maxInteractions) {
            int toRemove = (int)interactions.size() - maxInteractions;
            interactions.erase(interactions.begin(), interactions.begin() + toRemove);
            std::cout << "[MEMORY] Pruned " << toRemove << " oldest interactions.\n";
        }
        if ((int)qaHistory.size() > maxQAEntries) {
            int toRemove = (int)qaHistory.size() - maxQAEntries;
            qaHistory.erase(qaHistory.begin(), qaHistory.begin() + toRemove);
            std::cout << "[MEMORY] Pruned " << toRemove << " oldest Q&A entries.\n";
        }
    }

public:
    MKPersistentMemory(const std::string& filePath = "mk_memory.dat",
                       int maxInteract = 10000, int maxQA = 5000)
        : memoryFilePath(filePath), maxInteractions(maxInteract),
          maxQAEntries(maxQA), dirty(false) {
        std::cout << "[MEMORY] Persistent memory module initialized.\n";
        std::cout << "[MEMORY] Storage: " << memoryFilePath
                  << " | Max interactions: " << maxInteractions
                  << " | Max Q&A: " << maxQAEntries << "\n";
    }

    // ─── Load from File (call on boot) ─────────────────────────────────────────
    bool loadFromDisk() {
        std::ifstream file(memoryFilePath);
        if (!file.is_open()) {
            std::cout << "[MEMORY] No existing memory file found. Starting fresh.\n";
            return false;
        }

        int loadedInteractions = 0;
        int loadedPreferences = 0;
        int loadedQA = 0;

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;

            if (line.substr(0, 14) == "[INTERACTION] ") {
                std::string data = line.substr(14);
                auto parts = splitByPipe(data);
                if (parts.size() >= 3) {
                    MKInteraction entry;
                    entry.timestamp = parseTime(parts[0]);
                    entry.type = unescapePipe(parts[1]);
                    entry.content = unescapePipe(parts[2]);
                    interactions.push_back(entry);
                    loadedInteractions++;
                }
            } else if (line.substr(0, 13) == "[PREFERENCE] ") {
                std::string data = line.substr(13);
                auto parts = splitByPipe(data);
                if (parts.size() >= 4) {
                    MKPreference pref;
                    pref.key = unescapePipe(parts[0]);
                    pref.value = unescapePipe(parts[1]);
                    try { pref.confidence = std::stof(parts[2]); } catch (...) { pref.confidence = 0.5f; }
                    pref.lastUpdated = parseTime(parts[3]);
                    preferences[pref.key] = pref;
                    loadedPreferences++;
                }
            } else if (line.substr(0, 5) == "[QA] ") {
                std::string data = line.substr(5);
                auto parts = splitByPipe(data);
                if (parts.size() >= 4) {
                    MKQAEntry qa;
                    qa.timestamp = parseTime(parts[0]);
                    qa.question = unescapePipe(parts[1]);
                    qa.answer = unescapePipe(parts[2]);
                    try { qa.confidence = std::stof(parts[3]); } catch (...) { qa.confidence = 0.5f; }
                    qaHistory.push_back(qa);
                    loadedQA++;
                }
            }
        }

        file.close();
        pruneIfNeeded();

        std::cout << "[MEMORY] Loaded: " << loadedInteractions << " interactions, "
                  << loadedPreferences << " preferences, "
                  << loadedQA << " Q&A entries.\n";
        return true;
    }

    // ─── Save to File (call on shutdown) ───────────────────────────────────────
    bool saveToDisk() {
        std::ofstream file(memoryFilePath);
        if (!file.is_open()) {
            std::cerr << "[MEMORY ERROR] Cannot write to: " << memoryFilePath << "\n";
            return false;
        }

        file << "# MK Persistent Memory - Auto-generated\n";
        file << "# Format: [TYPE] field1|field2|...\n";
        file << "# Last saved: " << formatTime(std::time(nullptr)) << "\n\n";

        // Write interactions
        for (const auto& entry : interactions) {
            file << "[INTERACTION] " << entry.timestamp << "|"
                 << escapePipe(entry.type) << "|"
                 << escapePipe(entry.content) << "\n";
        }

        // Write preferences
        for (const auto& pair : preferences) {
            const auto& pref = pair.second;
            file << "[PREFERENCE] " << escapePipe(pref.key) << "|"
                 << escapePipe(pref.value) << "|"
                 << pref.confidence << "|"
                 << pref.lastUpdated << "\n";
        }

        // Write Q&A history
        for (const auto& qa : qaHistory) {
            file << "[QA] " << qa.timestamp << "|"
                 << escapePipe(qa.question) << "|"
                 << escapePipe(qa.answer) << "|"
                 << qa.confidence << "\n";
        }

        file.close();
        dirty = false;
        std::cout << "[MEMORY] Saved " << interactions.size() << " interactions, "
                  << preferences.size() << " preferences, "
                  << qaHistory.size() << " Q&A entries to disk.\n";
        return true;
    }

    // ─── Record Interaction ────────────────────────────────────────────────────
    void recordInteraction(const std::string& type, const std::string& content) {
        MKInteraction entry;
        entry.timestamp = std::time(nullptr);
        entry.type = type;
        entry.content = content;
        interactions.push_back(entry);
        dirty = true;
        pruneIfNeeded();
    }

    // ─── Track/Update Preference ───────────────────────────────────────────────
    void updatePreference(const std::string& key, const std::string& value) {
        if (preferences.find(key) != preferences.end()) {
            auto& pref = preferences[key];
            if (pref.value == value) {
                // Same preference observed again, increase confidence
                pref.confidence = std::min(1.0f, pref.confidence + 0.1f);
            } else {
                // Preference changed, reset confidence
                pref.value = value;
                pref.confidence = 0.5f;
            }
            pref.lastUpdated = std::time(nullptr);
        } else {
            MKPreference pref;
            pref.key = key;
            pref.value = value;
            pref.confidence = 0.5f;
            pref.lastUpdated = std::time(nullptr);
            preferences[key] = pref;
        }
        dirty = true;
    }

    // Get a preference value (returns empty if not found)
    std::string getPreference(const std::string& key) const {
        auto it = preferences.find(key);
        if (it != preferences.end()) return it->second.value;
        return "";
    }

    // Get preference confidence
    float getPreferenceConfidence(const std::string& key) const {
        auto it = preferences.find(key);
        if (it != preferences.end()) return it->second.confidence;
        return 0.0f;
    }

    // ─── Record Q&A ────────────────────────────────────────────────────────────
    void recordQA(const std::string& question, const std::string& answer, float confidence) {
        MKQAEntry qa;
        qa.timestamp = std::time(nullptr);
        qa.question = question;
        qa.answer = answer;
        qa.confidence = confidence;
        qaHistory.push_back(qa);
        dirty = true;
        pruneIfNeeded();
    }

    // Search previous Q&A for similar questions
    std::vector<MKQAEntry> findSimilarQuestions(const std::string& query, int maxResults = 5) {
        std::vector<MKQAEntry> results;
        std::string lowerQuery = query;
        std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);

        // Simple keyword matching (efficient for low-end hardware)
        for (auto it = qaHistory.rbegin(); it != qaHistory.rend() && (int)results.size() < maxResults; ++it) {
            std::string lowerQ = it->question;
            std::transform(lowerQ.begin(), lowerQ.end(), lowerQ.begin(), ::tolower);

            // Check if query words appear in stored question
            std::istringstream iss(lowerQuery);
            std::string word;
            int matchCount = 0;
            int totalWords = 0;
            while (iss >> word) {
                totalWords++;
                if (word.size() > 2 && lowerQ.find(word) != std::string::npos) {
                    matchCount++;
                }
            }

            if (totalWords > 0 && (float)matchCount / (float)totalWords > 0.4f) {
                results.push_back(*it);
            }
        }

        return results;
    }

    // ─── Query Methods ─────────────────────────────────────────────────────────

    // Get recent interactions
    std::vector<MKInteraction> getRecentInteractions(int count = 10) const {
        std::vector<MKInteraction> recent;
        int start = std::max(0, (int)interactions.size() - count);
        for (int i = start; i < (int)interactions.size(); i++) {
            recent.push_back(interactions[i]);
        }
        return recent;
    }

    // Get all preferences
    std::vector<MKPreference> getAllPreferences() const {
        std::vector<MKPreference> result;
        for (const auto& pair : preferences) {
            result.push_back(pair.second);
        }
        return result;
    }

    // Check if memory has unsaved changes
    bool hasDirtyState() const { return dirty; }

    // Get memory usage stats
    int getInteractionCount() const { return (int)interactions.size(); }
    int getPreferenceCount() const { return (int)preferences.size(); }
    int getQACount() const { return (int)qaHistory.size(); }

    // ─── Configuration ─────────────────────────────────────────────────────────
    void setMaxInteractions(int max) { maxInteractions = max; pruneIfNeeded(); }
    void setMaxQAEntries(int max) { maxQAEntries = max; pruneIfNeeded(); }
    void setMemoryFilePath(const std::string& path) { memoryFilePath = path; }

    void printStats() const {
        std::cout << "[MEMORY STATS] Interactions: " << interactions.size()
                  << "/" << maxInteractions
                  << " | Preferences: " << preferences.size()
                  << " | Q&A History: " << qaHistory.size()
                  << "/" << maxQAEntries
                  << " | Dirty: " << (dirty ? "yes" : "no") << "\n";
    }
};

#endif // MK_PERSISTENT_MEMORY_CPP
