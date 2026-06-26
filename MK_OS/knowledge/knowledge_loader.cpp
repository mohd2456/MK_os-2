#ifndef MK_KNOWLEDGE_LOADER_CPP
#define MK_KNOWLEDGE_LOADER_CPP

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <unordered_map>
#include <vector>

// Structural token containing the linguistic and semantic attributes of a concept
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

    // Helper to trim trailing carriage returns or white spaces from parsed strings
    std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

public:
    MKKnowledgeDataset() {
        std::cout << "[KNOWLEDGE] Advanced multi-token matrix engine ready.\n";
    }

    // Parses the relational schema from the raw text file
    bool loadDatabase() {
        std::ifstream file(databasePath);
        if (!file.is_open()) {
            std::cerr << "[KNOWLEDGE ERROR] Missing database layout at: " << databasePath << "\n";
            return false;
        }

        std::string line;
        while (std::getline(file, line)) {
            line = trim(line);
            // Bypass empty lines and syntax comment lines
            if (line.empty() || line[0] == '#') continue;

            std::stringstream ss(line);
            std::string key, pos, domain, weightStr, desc;

            // Split line sequentially by the pipe delimiter '|'
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
                    entry.contextWeight = 0.0; // Fail-safe default
                }
                
                entry.description = trim(desc);

                // Register entry into fast-lookup table map
                dataset[entry.key] = entry;
            }
        }

        file.close();
        std::cout << "[KNOWLEDGE] Matrix synchronized successfully. " << dataset.size() << " relational concepts mapped.\n";
        return true;
    }

    // Queries the memory layer and returns structured information
    void query(const std::string& concept) const {
        auto it = dataset.find(concept);
        if (it != dataset.end()) {
            const auto& entry = it->second;
            std::cout << "\n--- [KNOWLEDGE RETRIEVAL MATCH] ---\n";
            std::cout << "  Token:       " << entry.key << "\n";
            std::cout << "  Grammar:     " << entry.partOfSpeech << "\n";
            std::cout << "  Domain Context: [" << entry.domain << "] (Weight: " << entry.contextWeight << ")\n";
            std::cout << "  Definition:  " << entry.description << "\n";
            std::cout << "-----------------------------------\n";
        } else {
            std::cout << "[KNOWLEDGE MISS] Token \"" << concept << "\" does not exist within the data array.\n";
        }
    }
};

#endif // MK_KNOWLEDGE_LOADER_CPP