#ifndef MK_MEMORY_STORE_CPP
#define MK_MEMORY_STORE_CPP

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>

// ============================================================================
// MKMemoryStore - Permanent cross-session memory with flat file persistence
// Stores facts, preferences, history, and reminders with keyword search
// ============================================================================

enum class MemoryCategory {
    FACTS,
    PREFERENCES,
    HISTORY,
    REMINDERS,
    GENERAL
};

struct MemoryEntry {
    std::string key;
    std::string value;
    MemoryCategory category;
    std::string timestamp;
    std::vector<std::string> tags;
    int access_count;
    std::string last_accessed;
};

struct SearchHit {
    MemoryEntry entry;
    float relevance;
};

class MKMemoryStore {
private:
    std::string storage_path_;
    std::map<std::string, MemoryEntry> memories_;
    std::map<MemoryCategory, std::vector<std::string>> category_index_;
    bool dirty_;

    std::string getCurrentTimestamp() const {
        auto now = std::chrono::system_clock::now();
        time_t time = std::chrono::system_clock::to_time_t(now);
        std::string ts = std::ctime(&time);
        if (!ts.empty() && ts.back() == '\n') ts.pop_back();
        return ts;
    }

    std::string categoryToString(MemoryCategory cat) const {
        switch (cat) {
            case MemoryCategory::FACTS: return "facts";
            case MemoryCategory::PREFERENCES: return "preferences";
            case MemoryCategory::HISTORY: return "history";
            case MemoryCategory::REMINDERS: return "reminders";
            default: return "general";
        }
    }

    MemoryCategory stringToCategory(const std::string& str) const {
        if (str == "facts") return MemoryCategory::FACTS;
        if (str == "preferences") return MemoryCategory::PREFERENCES;
        if (str == "history") return MemoryCategory::HISTORY;
        if (str == "reminders") return MemoryCategory::REMINDERS;
        return MemoryCategory::GENERAL;
    }

    std::string toLower(const std::string& str) const {
        std::string lower = str;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        return lower;
    }

    std::vector<std::string> tokenize(const std::string& text) const {
        std::vector<std::string> tokens;
        std::istringstream stream(toLower(text));
        std::string word;
        while (stream >> word) {
            word.erase(std::remove_if(word.begin(), word.end(), ::ispunct), word.end());
            if (!word.empty()) tokens.push_back(word);
        }
        return tokens;
    }

    float computeRelevance(const std::string& query, const MemoryEntry& entry) const {
        std::vector<std::string> query_tokens = tokenize(query);
        std::vector<std::string> key_tokens = tokenize(entry.key);
        std::vector<std::string> value_tokens = tokenize(entry.value);

        float score = 0.0f;
        for (const auto& qt : query_tokens) {
            // Check key (higher weight)
            for (const auto& kt : key_tokens) {
                if (kt == qt) score += 3.0f;
                else if (kt.find(qt) != std::string::npos) score += 1.5f;
            }
            // Check value
            for (const auto& vt : value_tokens) {
                if (vt == qt) score += 1.0f;
                else if (vt.find(qt) != std::string::npos) score += 0.5f;
            }
            // Check tags
            for (const auto& tag : entry.tags) {
                if (toLower(tag) == qt) score += 2.0f;
            }
        }

        // Normalize
        if (!query_tokens.empty()) {
            score /= static_cast<float>(query_tokens.size());
        }
        return score;
    }

    std::string escapeValue(const std::string& str) const {
        std::string escaped;
        for (char c : str) {
            if (c == '\n') escaped += "\\n";
            else if (c == '\t') escaped += "\\t";
            else if (c == '|') escaped += "\\|";
            else escaped += c;
        }
        return escaped;
    }

    std::string unescapeValue(const std::string& str) const {
        std::string unescaped;
        for (size_t i = 0; i < str.size(); i++) {
            if (str[i] == '\\' && i + 1 < str.size()) {
                if (str[i+1] == 'n') { unescaped += '\n'; i++; }
                else if (str[i+1] == 't') { unescaped += '\t'; i++; }
                else if (str[i+1] == '|') { unescaped += '|'; i++; }
                else unescaped += str[i];
            } else {
                unescaped += str[i];
            }
        }
        return unescaped;
    }

public:
    MKMemoryStore() : storage_path_("./mk_memory.dat"), dirty_(false) {}

    explicit MKMemoryStore(const std::string& path) : storage_path_(path), dirty_(false) {}

    void remember(const std::string& key, const std::string& value, 
                  MemoryCategory category = MemoryCategory::GENERAL,
                  const std::vector<std::string>& tags = {}) {
        MemoryEntry entry;
        entry.key = key;
        entry.value = value;
        entry.category = category;
        entry.timestamp = getCurrentTimestamp();
        entry.tags = tags;
        entry.access_count = 0;
        entry.last_accessed = entry.timestamp;

        memories_[key] = entry;
        category_index_[category].push_back(key);
        dirty_ = true;
    }

    std::string recall(const std::string& key) {
        auto it = memories_.find(key);
        if (it != memories_.end()) {
            it->second.access_count++;
            it->second.last_accessed = getCurrentTimestamp();
            return it->second.value;
        }
        // Try case-insensitive search
        std::string lower_key = toLower(key);
        for (auto& kv : memories_) {
            if (toLower(kv.first) == lower_key) {
                kv.second.access_count++;
                kv.second.last_accessed = getCurrentTimestamp();
                return kv.second.value;
            }
        }
        return "";
    }

    std::vector<SearchHit> search(const std::string& query, int max_results = 10) const {
        std::vector<SearchHit> results;

        for (const auto& kv : memories_) {
            float relevance = computeRelevance(query, kv.second);
            if (relevance > 0.5f) {
                results.push_back({kv.second, relevance});
            }
        }

        std::sort(results.begin(), results.end(),
                  [](const SearchHit& a, const SearchHit& b) {
                      return a.relevance > b.relevance;
                  });

        if (static_cast<int>(results.size()) > max_results) {
            results.resize(max_results);
        }
        return results;
    }

    std::map<std::string, std::string> getUserPreferences() const {
        std::map<std::string, std::string> prefs;
        auto it = category_index_.find(MemoryCategory::PREFERENCES);
        if (it != category_index_.end()) {
            for (const auto& key : it->second) {
                auto mem_it = memories_.find(key);
                if (mem_it != memories_.end()) {
                    prefs[key] = mem_it->second.value;
                }
            }
        }
        return prefs;
    }

    bool save() {
        std::ofstream file(storage_path_);
        if (!file.is_open()) {
            std::cerr << "[MKMemoryStore] Failed to save to: " << storage_path_ << std::endl;
            return false;
        }

        file << "# MK Memory Store v1.0\n";
        for (const auto& kv : memories_) {
            const auto& entry = kv.second;
            // Format: key|value|category|timestamp|access_count|tags
            file << escapeValue(entry.key) << "|"
                 << escapeValue(entry.value) << "|"
                 << categoryToString(entry.category) << "|"
                 << entry.timestamp << "|"
                 << entry.access_count << "|";
            for (size_t i = 0; i < entry.tags.size(); i++) {
                file << entry.tags[i];
                if (i < entry.tags.size() - 1) file << ",";
            }
            file << "\n";
        }
        file.close();
        dirty_ = false;
        return true;
    }

    bool load() {
        std::ifstream file(storage_path_);
        if (!file.is_open()) return false;

        memories_.clear();
        category_index_.clear();

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;

            std::vector<std::string> parts;
            std::istringstream stream(line);
            std::string part;
            while (std::getline(stream, part, '|')) {
                parts.push_back(part);
            }

            if (parts.size() >= 4) {
                MemoryEntry entry;
                entry.key = unescapeValue(parts[0]);
                entry.value = unescapeValue(parts[1]);
                entry.category = stringToCategory(parts[2]);
                entry.timestamp = parts[3];
                entry.access_count = (parts.size() > 4) ? std::stoi(parts[4]) : 0;
                if (parts.size() > 5 && !parts[5].empty()) {
                    std::istringstream tag_stream(parts[5]);
                    std::string tag;
                    while (std::getline(tag_stream, tag, ',')) {
                        entry.tags.push_back(tag);
                    }
                }
                memories_[entry.key] = entry;
                category_index_[entry.category].push_back(entry.key);
            }
        }
        file.close();
        dirty_ = false;
        return true;
    }

    bool forget(const std::string& key) {
        auto it = memories_.find(key);
        if (it != memories_.end()) {
            MemoryCategory cat = it->second.category;
            memories_.erase(it);
            auto& idx = category_index_[cat];
            idx.erase(std::remove(idx.begin(), idx.end(), key), idx.end());
            dirty_ = true;
            return true;
        }
        return false;
    }

    int getMemoryCount() const { return static_cast<int>(memories_.size()); }
    bool isDirty() const { return dirty_; }
    std::string getStoragePath() const { return storage_path_; }

    std::vector<MemoryEntry> getByCategory(MemoryCategory cat) const {
        std::vector<MemoryEntry> results;
        auto it = category_index_.find(cat);
        if (it != category_index_.end()) {
            for (const auto& key : it->second) {
                auto mem_it = memories_.find(key);
                if (mem_it != memories_.end()) {
                    results.push_back(mem_it->second);
                }
            }
        }
        return results;
    }

    void clear() {
        memories_.clear();
        category_index_.clear();
        dirty_ = true;
    }
};

#endif // MK_MEMORY_STORE_CPP
