#ifndef MK_CHAIN_REACTOR_CPP
#define MK_CHAIN_REACTOR_CPP

#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <iostream>
#include <cmath>
#include "../safe_parse.h"

// ===================================================================================
// MK CONSCIOUSNESS ENGINE — LAYER 2: CHAIN REACTOR
// ===================================================================================
// The "thinking" layer. Input atoms trigger related atoms from memory.
// Like a graph search on MEANING. "physics" → "exam" → "stressed" → "encourage"
// because those atoms co-occurred in past conversations.
// ===================================================================================

// Forward declaration — we reference MKPatternGraph for graph-based connections
// (The actual class is defined in pattern_graph.cpp, included before this)
#ifndef MK_PATTERN_GRAPH_CPP
class MKPatternGraph;
#endif

struct MKReactedAtom {
    MKThoughtAtom atom;
    float relevanceScore;  // combined score from distance, frequency, recency
    int hopDistance;        // how far from the input atom
};

class MKChainReactor {
private:
    std::vector<MKThoughtAtom> memory_;
    std::unordered_map<std::string, std::vector<int>> atom_index_; // value → indexes
    static const size_t MAX_MEMORY = 50000;

    // Score an atom by how relevant it is to the current context
    float scoreAtom(const MKThoughtAtom& atom, int hopDistance, int frequency, long long now) const {
        // Distance: closer hops = higher score
        float distScore = 1.0f / (1.0f + (float)hopDistance);

        // Frequency: seen more = higher
        float freqScore = std::min(1.0f, (float)frequency / 10.0f);

        // Recency: newer = higher (decay over time)
        float age = (float)(now - atom.timestamp) / 1000000000.0f; // rough seconds
        float recencyScore = 1.0f / (1.0f + age * 0.001f);

        // Weight from atom itself
        float weightScore = atom.weight;

        return (distScore * 0.4f + freqScore * 0.2f + recencyScore * 0.2f + weightScore * 0.2f);
    }

    // Find all atoms in memory with matching value
    std::vector<int> findByValue(const std::string& value) const {
        auto it = atom_index_.find(value);
        if (it != atom_index_.end()) return it->second;
        return {};
    }

    // Find atoms related through the pattern graph
    std::vector<std::string> getGraphRelated(const std::string& value, MKPatternGraph& graph) const {
        std::vector<std::string> related;
        // Get all edges from this concept
        auto edges = graph.getAll(value);
        for (const auto& e : edges) {
            related.push_back(e.target);
        }
        // Also check reverse — what points TO this value
        // Use common relations
        static const std::vector<std::string> commonRelations = {"is_a", "has", "related_to", "part_of", "can"};
        for (const auto& rel : commonRelations) {
            auto reverseResults = graph.reverseQuery(rel, value);
            for (const auto& r : reverseResults) {
                related.push_back(r);
            }
        }
        return related;
    }

public:
    MKChainReactor() {
        memory_.reserve(1000);
        std::cout << "[MCE:CHAIN_REACTOR] initialized\n";
    }

    // Store new atoms in memory
    void addAtoms(const std::vector<MKThoughtAtom>& atoms) {
        for (const auto& atom : atoms) {
            int idx = (int)memory_.size();
            memory_.push_back(atom);
            atom_index_[atom.value].push_back(idx);
        }
        // Prune if over limit
        if (memory_.size() > MAX_MEMORY) {
            prune(MAX_MEMORY / 2);
        }
    }

    // Main method: trigger a chain reaction from input atoms
    std::vector<MKReactedAtom> react(const std::vector<MKThoughtAtom>& inputAtoms,
                                      MKPatternGraph& graph, int maxHops = 3) {
        std::vector<MKReactedAtom> results;
        std::unordered_map<std::string, float> seen; // prevent duplicates
        long long now = std::chrono::steady_clock::now().time_since_epoch().count();

        // Hop 1: Direct matches from input atoms
        for (const auto& inputAtom : inputAtoms) {
            // Skip low-info atoms (tone, certainty with low weight)
            if (inputAtom.weight < 0.3f) continue;

            // Find same-value atoms in memory
            auto indexes = findByValue(inputAtom.value);
            int frequency = (int)indexes.size();

            for (int idx : indexes) {
                const auto& memAtom = memory_[idx];
                float score = scoreAtom(memAtom, 1, frequency, now);
                if (seen.find(memAtom.value + std::to_string((int)memAtom.type)) == seen.end()) {
                    results.push_back({memAtom, score, 1});
                    seen[memAtom.value + std::to_string((int)memAtom.type)] = score;
                }
            }

            // Find graph-related concepts
            auto graphRelated = getGraphRelated(inputAtom.value, graph);
            for (const auto& related : graphRelated) {
                auto relIndexes = findByValue(related);
                int relFreq = (int)relIndexes.size();
                for (int idx : relIndexes) {
                    const auto& memAtom = memory_[idx];
                    float score = scoreAtom(memAtom, 1, relFreq, now);
                    std::string key = memAtom.value + std::to_string((int)memAtom.type);
                    if (seen.find(key) == seen.end()) {
                        results.push_back({memAtom, score * 0.8f, 1});
                        seen[key] = score * 0.8f;
                    }
                }
            }
        }

        // Hop 2+: For each found atom, find ITS related atoms
        if (maxHops >= 2) {
            std::vector<MKReactedAtom> hop1Results = results; // snapshot
            for (const auto& hop1 : hop1Results) {
                if (hop1.relevanceScore < 0.2f) continue; // skip weak connections

                auto indexes = findByValue(hop1.atom.value);
                // Look at neighbors (atoms stored near this one were likely in same conversation)
                for (int idx : indexes) {
                    // Check neighbors within +-3 positions in memory
                    for (int offset = -3; offset <= 3; offset++) {
                        int neighborIdx = idx + offset;
                        if (neighborIdx < 0 || neighborIdx >= (int)memory_.size() || neighborIdx == idx)
                            continue;
                        const auto& neighbor = memory_[neighborIdx];
                        std::string key = neighbor.value + std::to_string((int)neighbor.type);
                        if (seen.find(key) == seen.end()) {
                            float score = scoreAtom(neighbor, 2, 1, now) * 0.5f;
                            results.push_back({neighbor, score, 2});
                            seen[key] = score;
                        }
                    }
                }

                // Also search graph for hop2 connections
                if (maxHops >= 3) {
                    auto graphRelated = getGraphRelated(hop1.atom.value, graph);
                    for (const auto& related : graphRelated) {
                        auto relIndexes = findByValue(related);
                        for (int idx : relIndexes) {
                            const auto& memAtom = memory_[idx];
                            std::string key = memAtom.value + std::to_string((int)memAtom.type);
                            if (seen.find(key) == seen.end()) {
                                float score = scoreAtom(memAtom, 3, 1, now) * 0.3f;
                                results.push_back({memAtom, score, 3});
                                seen[key] = score;
                            }
                        }
                    }
                }
            }
        }

        // Sort by relevance and return top 20
        std::sort(results.begin(), results.end(), [](const MKReactedAtom& a, const MKReactedAtom& b) {
            return a.relevanceScore > b.relevanceScore;
        });

        if (results.size() > 20) results.resize(20);
        return results;
    }

    // Find atoms directly related to a single atom
    std::vector<MKReactedAtom> findRelatedAtoms(const MKThoughtAtom& atom, MKPatternGraph& graph) {
        std::vector<MKThoughtAtom> single = {atom};
        return react(single, graph, 1);
    }

    size_t getMemorySize() const { return memory_.size(); }

    void prune(size_t maxSize) {
        if (memory_.size() <= maxSize) return;
        // Keep the most recent atoms
        size_t removeCount = memory_.size() - maxSize;
        memory_.erase(memory_.begin(), memory_.begin() + removeCount);
        // Rebuild index
        atom_index_.clear();
        for (int i = 0; i < (int)memory_.size(); i++) {
            atom_index_[memory_[i].value].push_back(i);
        }
    }

    // Save memory to disk
    void save(const std::string& filename) const {
        std::ofstream out(filename);
        if (!out.is_open()) return;
        for (const auto& atom : memory_) {
            out << (int)atom.type << "|" << atom.value << "|" << atom.weight
                << "|" << atom.timestamp << "|" << atom.source << "\n";
        }
        out.close();
    }

    // Load memory from disk
    void load(const std::string& filename) {
        std::ifstream in(filename);
        if (!in.is_open()) return;
        memory_.clear();
        atom_index_.clear();
        std::string line;
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            std::istringstream ss(line);
            std::string typeStr, value, weightStr, tsStr, source;
            if (std::getline(ss, typeStr, '|') && std::getline(ss, value, '|') &&
                std::getline(ss, weightStr, '|') && std::getline(ss, tsStr, '|') &&
                std::getline(ss, source, '|')) {
                MKThoughtAtom atom;
                atom.type = static_cast<MKAtomType>(mk::safeStoi(typeStr, 0));
                atom.value = value;
                atom.weight = mk::safeStof(weightStr, 0.0f);
                atom.timestamp = mk::safeStoll(tsStr, 0);
                atom.source = source;
                int idx = (int)memory_.size();
                memory_.push_back(atom);
                atom_index_[value].push_back(idx);
            }
        }
        in.close();
    }
};

#endif // MK_CHAIN_REACTOR_CPP
