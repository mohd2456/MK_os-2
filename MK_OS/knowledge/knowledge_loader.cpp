#ifndef MK_KNOWLEDGE_LOADER_CPP
#define MK_KNOWLEDGE_LOADER_CPP

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <algorithm>

// ===================================================================================
// MK KNOWLEDGE DATASET — Linguistic & Definition Layer
// ===================================================================================
// This module loads the DICTIONARY-style knowledge from knowledge/knowledge_data.txt
// (5-field format: key|POS|domain|weight|description)
//
// It also BRIDGES into the main reasoning graph by converting entries to graph format
// (4-field: source|relation|target|weight) and appending them to the HRE knowledge files.
//
// ARCHITECTURE:
//   knowledge/knowledge_data.txt  →  MKKnowledgeDataset (this class)
//                                         ↓ convertToGraphFormat()
//                                 ai_core/hre/knowledge_files/core_facts.mk
//
// This ensures NOTHING in knowledge/ is wasted — it ALL feeds into the brain.
// ===================================================================================

struct KnowledgeEntry {
    std::string key;
    std::string partOfSpeech;
    std::string domain;
    double contextWeight;
    std::string description;
};

class MKKnowledgeDataset {
private:
    std::unordered_map<std::string, KnowledgeEntry> dataset;
    std::string databasePath = "knowledge/knowledge_data.txt";
    std::string graphOutputPath = "ai_core/hre/knowledge_files/core_facts.mk";

    std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

    std::string toLower(const std::string& s) {
        std::string result = s;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

    // Replace spaces with underscores for graph node names
    std::string toNodeName(const std::string& s) {
        std::string result = toLower(trim(s));
        std::replace(result.begin(), result.end(), ' ', '_');
        return result;
    }

public:
    MKKnowledgeDataset() {
        std::cout << "[KNOWLEDGE] Linguistic & definition dataset engine ready.\n";
    }

    // Set custom paths (for when running from different directories)
    void setDatabasePath(const std::string& path) { databasePath = path; }
    void setGraphOutputPath(const std::string& path) { graphOutputPath = path; }

    // Parses the 5-field format from knowledge_data.txt
    bool loadDatabase() {
        std::ifstream file(databasePath);
        if (!file.is_open()) {
            std::cerr << "[KNOWLEDGE ERROR] Missing database at: " << databasePath << "\n";
            return false;
        }

        std::string line;
        while (std::getline(file, line)) {
            line = trim(line);
            if (line.empty() || line[0] == '#') continue;

            std::stringstream ss(line);
            std::string key, pos, domain, weightStr, desc;

            if (std::getline(ss, key, '|') &&
                std::getline(ss, pos, '|') &&
                std::getline(ss, domain, '|') &&
                std::getline(ss, weightStr, '|') &&
                std::getline(ss, desc, '|')) {

                KnowledgeEntry entry;
                entry.key = trim(key);
                entry.partOfSpeech = trim(pos);
                entry.domain = trim(domain);
                
                try {
                    entry.contextWeight = std::stod(trim(weightStr));
                } catch (...) {
                    entry.contextWeight = 0.5;
                }
                
                entry.description = trim(desc);
                dataset[entry.key] = entry;
            }
        }

        file.close();
        std::cout << "[KNOWLEDGE] Loaded " << dataset.size() << " dictionary entries.\n";
        return true;
    }

    // Query a concept from the dictionary
    void query(const std::string& concept) const {
        auto it = dataset.find(concept);
        if (it != dataset.end()) {
            const auto& entry = it->second;
            std::cout << "\n--- [KNOWLEDGE RETRIEVAL] ---\n";
            std::cout << "  Token:      " << entry.key << "\n";
            std::cout << "  Grammar:    " << entry.partOfSpeech << "\n";
            std::cout << "  Domain:     [" << entry.domain << "] (Weight: " << entry.contextWeight << ")\n";
            std::cout << "  Definition: " << entry.description << "\n";
            std::cout << "-----------------------------\n";
        } else {
            std::cout << "[KNOWLEDGE MISS] \"" << concept << "\" not in dictionary.\n";
        }
    }

    // Get entry by key (for programmatic access)
    const KnowledgeEntry* getEntry(const std::string& key) const {
        auto it = dataset.find(key);
        return (it != dataset.end()) ? &it->second : nullptr;
    }

    // Get all entries in a domain
    std::vector<KnowledgeEntry> getByDomain(const std::string& domain) const {
        std::vector<KnowledgeEntry> results;
        for (const auto& kv : dataset) {
            if (kv.second.domain == domain) results.push_back(kv.second);
        }
        return results;
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  BRIDGE: Convert dictionary entries to graph format (.mk)
    // ─────────────────────────────────────────────────────────────────────────
    // This is what connects knowledge/ to ai_core/hre/knowledge_files/
    // Every dictionary entry becomes graph facts that the reasoning engine uses.

    int convertToGraphFormat(const std::string& outputFile = "") {
        std::string outPath = outputFile.empty() ? graphOutputPath : outputFile;
        std::ofstream out(outPath, std::ios::app);
        if (!out.is_open()) {
            std::cerr << "[KNOWLEDGE] Cannot write graph output to: " << outPath << "\n";
            return 0;
        }

        int converted = 0;
        out << "\n# ═══ Auto-imported from knowledge/knowledge_data.txt ═══\n";
        out << "# These facts were converted from the dictionary format.\n\n";

        for (const auto& kv : dataset) {
            const auto& entry = kv.second;
            std::string node = toNodeName(entry.key);
            std::string weight = std::to_string(entry.contextWeight).substr(0, 4);

            // Fact 1: X is_a [part_of_speech]
            out << node << "|is_a|" << toLower(entry.partOfSpeech) << "|" << weight << "\n";
            converted++;

            // Fact 2: X belongs_to_domain [domain]
            out << node << "|belongs_to_domain|" << toLower(entry.domain) << "|" << weight << "\n";
            converted++;

            // Fact 3: X defined_as [first 80 chars of description]
            std::string shortDesc = entry.description.substr(0, 80);
            std::replace(shortDesc.begin(), shortDesc.end(), '|', '-');
            std::replace(shortDesc.begin(), shortDesc.end(), '\n', ' ');
            out << node << "|defined_as|" << toLower(shortDesc) << "|" << weight << "\n";
            converted++;
        }

        out.close();
        std::cout << "[KNOWLEDGE → GRAPH] Converted " << converted 
                  << " facts from dictionary into graph format.\n";
        return converted;
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  BRIDGE: Convert datasets/ loaded data into graph format
    // ─────────────────────────────────────────────────────────────────────────
    // Call this with data loaded from datasets/loader.cpp or parser.cpp

    int importExternalData(const std::vector<std::pair<std::string, std::string>>& keyValuePairs,
                           const std::string& sourceName,
                           const std::string& outputFile = "") {
        std::string outPath = outputFile.empty() ? "ai_core/hre/knowledge_files/learned_facts.mk" : outputFile;
        std::ofstream out(outPath, std::ios::app);
        if (!out.is_open()) return 0;

        int imported = 0;
        out << "\n# ═══ Auto-imported from datasets/ via " << sourceName << " ═══\n";

        for (const auto& kv : keyValuePairs) {
            std::string key = toNodeName(kv.first);
            std::string value = toLower(trim(kv.second));
            if (key.empty() || value.empty()) continue;
            
            // Store as a generic "has_value" relation
            out << key << "|has_value|" << value << "|0.7\n";
            imported++;
        }

        out.close();
        std::cout << "[KNOWLEDGE → GRAPH] Imported " << imported 
                  << " entries from " << sourceName << " into learned_facts.\n";
        return imported;
    }

    int size() const { return dataset.size(); }

    void printStats() const {
        std::cout << "[KNOWLEDGE STATS] Dictionary entries: " << dataset.size() << "\n";
        
        // Count by domain
        std::unordered_map<std::string, int> domainCounts;
        for (const auto& kv : dataset) {
            domainCounts[kv.second.domain]++;
        }
        for (const auto& dc : domainCounts) {
            std::cout << "  [" << dc.first << "] " << dc.second << " entries\n";
        }
    }
};

#endif // MK_KNOWLEDGE_LOADER_CPP