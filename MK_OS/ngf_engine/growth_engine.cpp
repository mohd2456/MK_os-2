#ifndef MK_GROWTH_ENGINE_CPP
#define MK_GROWTH_ENGINE_CPP

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <fstream>
#include <chrono>
#include <functional>

// ===================================================================================
// MK NEURAL GRAPH FUSION - GROWTH ENGINE
// ===================================================================================
// Every conversation adds new connections to the concept web. Every web search adds
// new nodes. This engine manages the growth of the knowledge graph over time.
//
// Features:
//   - Add new facts from conversations
//   - Bulk load from knowledge files (.mk format)
//   - Track frequently accessed concepts (hot cache)
//   - Persist graph state to disk
//   - Prune low-confidence or stale connections
//   - Statistics on growth over time
//   - Merge duplicate concepts
//   - Conflict resolution for contradictory facts
// ===================================================================================

// A record of when a fact was learned
struct MKGrowthRecord {
    std::string source;
    std::string relation;
    std::string target;
    float weight;
    float confidence;
    uint64_t learned_at;       // Timestamp when learned
    std::string origin;        // Where this came from (conversation, search, file)
    uint32_t access_count;     // How often this fact is accessed
    uint64_t last_accessed;    // Last access time

    MKGrowthRecord()
        : weight(0.5f), confidence(0.8f), learned_at(0),
          origin("unknown"), access_count(0), last_accessed(0) {}
};

// Hot cache entry
struct MKHotCacheEntry {
    std::string concept;
    uint32_t access_count;
    uint64_t last_accessed;
    float importance;

    MKHotCacheEntry() : access_count(0), last_accessed(0), importance(0.0f) {}
    MKHotCacheEntry(const std::string& c, uint32_t a, uint64_t t, float i)
        : concept(c), access_count(a), last_accessed(t), importance(i) {}
};

// Growth statistics
struct MKGrowthStats {
    size_t total_facts_learned;
    size_t facts_from_conversation;
    size_t facts_from_search;
    size_t facts_from_files;
    size_t facts_pruned;
    size_t conflicts_resolved;
    size_t duplicates_merged;
    uint64_t first_fact_time;
    uint64_t last_fact_time;
    size_t hot_cache_size;

    MKGrowthStats()
        : total_facts_learned(0), facts_from_conversation(0),
          facts_from_search(0), facts_from_files(0), facts_pruned(0),
          conflicts_resolved(0), duplicates_merged(0),
          first_fact_time(0), last_fact_time(0), hot_cache_size(0) {}
};

// The main growth engine class
class MKGrowthEngine {
private:
    // All learned facts
    std::vector<MKGrowthRecord> facts_;

    // Index: concept -> indices into facts_ where it appears as source or target
    std::unordered_map<std::string, std::vector<size_t>> concept_index_;

    // Hot cache: most frequently accessed concepts
    std::unordered_map<std::string, MKHotCacheEntry> hot_cache_;
    size_t hot_cache_max_size_;

    // Growth statistics
    MKGrowthStats stats_;

    // Persistence file path
    std::string persist_path_;

    // Conflict tracking
    std::unordered_map<std::string, std::vector<size_t>> contradictions_;

    // Get current timestamp
    uint64_t now() const {
        auto tp = std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::seconds>(
            tp.time_since_epoch()).count();
    }

    // Normalize concept name
    std::string normalize(const std::string& name) const {
        std::string result = name;
        size_t start = result.find_first_not_of(" \t\n\r");
        size_t end = result.find_last_not_of(" \t\n\r");
        if (start == std::string::npos) return "";
        result = result.substr(start, end - start + 1);
        std::transform(result.begin(), result.end(), result.begin(),
                      [](unsigned char c) { return std::tolower(c); });
        std::replace(result.begin(), result.end(), ' ', '_');
        return result;
    }

    // Update hot cache with an access
    void updateHotCache(const std::string& concept) {
        auto it = hot_cache_.find(concept);
        if (it != hot_cache_.end()) {
            it->second.access_count++;
            it->second.last_accessed = now();
        } else {
            if (hot_cache_.size() >= hot_cache_max_size_) {
                // Evict least accessed entry
                std::string evict_key;
                uint32_t min_access = UINT32_MAX;
                for (const auto& [key, entry] : hot_cache_) {
                    if (entry.access_count < min_access) {
                        min_access = entry.access_count;
                        evict_key = key;
                    }
                }
                if (!evict_key.empty()) {
                    hot_cache_.erase(evict_key);
                }
            }
            hot_cache_[concept] = MKHotCacheEntry(concept, 1, now(), 0.5f);
        }
    }

    // Check if a fact contradicts existing knowledge
    bool checkContradiction(const std::string& source, const std::string& relation,
                           const std::string& target) const {
        // Opposite relations that conflict
        static const std::unordered_map<std::string, std::string> opposites = {
            {"causes", "prevents"}, {"prevents", "causes"},
            {"enables", "prevents"}, {"stronger_than", "weaker_than"},
            {"weaker_than", "stronger_than"}, {"precedes", "follows"},
            {"follows", "precedes"}, {"supports", "contradicts"},
            {"contradicts", "supports"}
        };

        auto opp_it = opposites.find(relation);
        if (opp_it == opposites.end()) return false;

        // Check if source has the opposite relation to the same target
        auto idx_it = concept_index_.find(source);
        if (idx_it == concept_index_.end()) return false;

        for (size_t idx : idx_it->second) {
            if (idx < facts_.size()) {
                const auto& fact = facts_[idx];
                if (fact.source == source && fact.target == target &&
                    fact.relation == opp_it->second) {
                    return true;
                }
            }
        }
        return false;
    }

public:
    MKGrowthEngine() : hot_cache_max_size_(1000), persist_path_("ngf_knowledge.dat") {
        std::cout << "[NGF GrowthEngine] Initialized growth engine" << std::endl;
    }

    MKGrowthEngine(const std::string& persist_path, size_t cache_size = 1000)
        : hot_cache_max_size_(cache_size), persist_path_(persist_path) {
        std::cout << "[NGF GrowthEngine] Initialized with persist path: "
                  << persist_path << std::endl;
    }

    // --- Learning from conversations ---
    bool learnFromConversation(const std::string& source, const std::string& relation,
                               const std::string& target, float weight = 0.5f,
                               float confidence = 0.7f) {
        std::string src = normalize(source);
        std::string tgt = normalize(target);
        std::string rel = normalize(relation);

        if (src.empty() || tgt.empty() || rel.empty()) return false;

        // Check for contradictions
        if (checkContradiction(src, rel, tgt)) {
            stats_.conflicts_resolved++;
            // Lower confidence for contradicting facts
            confidence *= 0.5f;
        }

        MKGrowthRecord record;
        record.source = src;
        record.relation = rel;
        record.target = tgt;
        record.weight = weight;
        record.confidence = confidence;
        record.learned_at = now();
        record.origin = "conversation";
        record.access_count = 0;
        record.last_accessed = now();

        size_t idx = facts_.size();
        facts_.push_back(record);
        concept_index_[src].push_back(idx);
        concept_index_[tgt].push_back(idx);

        updateHotCache(src);
        updateHotCache(tgt);

        stats_.total_facts_learned++;
        stats_.facts_from_conversation++;
        if (stats_.first_fact_time == 0) stats_.first_fact_time = now();
        stats_.last_fact_time = now();

        return true;
    }

    // --- Learning from web searches ---
    bool learnFromSearch(const std::string& source, const std::string& relation,
                         const std::string& target, float weight = 0.6f,
                         float confidence = 0.75f) {
        std::string src = normalize(source);
        std::string tgt = normalize(target);
        std::string rel = normalize(relation);

        if (src.empty() || tgt.empty() || rel.empty()) return false;

        MKGrowthRecord record;
        record.source = src;
        record.relation = rel;
        record.target = tgt;
        record.weight = weight;
        record.confidence = confidence;
        record.learned_at = now();
        record.origin = "search";
        record.access_count = 0;
        record.last_accessed = now();

        size_t idx = facts_.size();
        facts_.push_back(record);
        concept_index_[src].push_back(idx);
        concept_index_[tgt].push_back(idx);

        updateHotCache(src);
        updateHotCache(tgt);

        stats_.total_facts_learned++;
        stats_.facts_from_search++;
        stats_.last_fact_time = now();

        return true;
    }

    // --- Bulk loading from .mk files ---
    size_t loadFromFile(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cout << "[NGF GrowthEngine] Failed to open: " << filepath << std::endl;
            return 0;
        }

        size_t loaded = 0;
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;

            std::istringstream iss(line);
            std::string source, relation, target, weight_str;

            if (std::getline(iss, source, '|') &&
                std::getline(iss, relation, '|') &&
                std::getline(iss, target, '|') &&
                std::getline(iss, weight_str)) {

                float weight = 0.5f;
                try { weight = std::stof(weight_str); } catch (...) {}

                MKGrowthRecord record;
                record.source = normalize(source);
                record.relation = normalize(relation);
                record.target = normalize(target);
                record.weight = weight;
                record.confidence = 0.9f;  // File-loaded facts get high confidence
                record.learned_at = now();
                record.origin = "file:" + filepath;

                if (!record.source.empty() && !record.target.empty()) {
                    size_t idx = facts_.size();
                    facts_.push_back(record);
                    concept_index_[record.source].push_back(idx);
                    concept_index_[record.target].push_back(idx);
                    loaded++;
                }
            }
        }

        stats_.total_facts_learned += loaded;
        stats_.facts_from_files += loaded;
        stats_.last_fact_time = now();

        std::cout << "[NGF GrowthEngine] Loaded " << loaded << " facts from " << filepath << std::endl;
        return loaded;
    }

    // --- Persistence ---
    bool persistToDisk() const {
        std::ofstream file(persist_path_);
        if (!file.is_open()) {
            std::cout << "[NGF GrowthEngine] Failed to open persist file: "
                      << persist_path_ << std::endl;
            return false;
        }

        file << "# MK NGF Growth Engine - Persisted Knowledge\n";
        file << "# Format: source|relation|target|weight|confidence|timestamp|origin\n";
        file << "# Total facts: " << facts_.size() << "\n";

        for (const auto& fact : facts_) {
            file << fact.source << "|" << fact.relation << "|" << fact.target
                 << "|" << fact.weight << "|" << fact.confidence
                 << "|" << fact.learned_at << "|" << fact.origin << "\n";
        }

        file.close();
        std::cout << "[NGF GrowthEngine] Persisted " << facts_.size()
                  << " facts to " << persist_path_ << std::endl;
        return true;
    }

    bool loadFromDisk() {
        std::ifstream file(persist_path_);
        if (!file.is_open()) return false;

        facts_.clear();
        concept_index_.clear();

        std::string line;
        size_t loaded = 0;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;

            std::istringstream iss(line);
            std::string source, relation, target, weight_str, conf_str, ts_str, origin;

            if (std::getline(iss, source, '|') &&
                std::getline(iss, relation, '|') &&
                std::getline(iss, target, '|') &&
                std::getline(iss, weight_str, '|') &&
                std::getline(iss, conf_str, '|') &&
                std::getline(iss, ts_str, '|') &&
                std::getline(iss, origin)) {

                MKGrowthRecord record;
                record.source = source;
                record.relation = relation;
                record.target = target;
                try { record.weight = std::stof(weight_str); } catch (...) { record.weight = 0.5f; }
                try { record.confidence = std::stof(conf_str); } catch (...) { record.confidence = 0.8f; }
                try { record.learned_at = std::stoull(ts_str); } catch (...) { record.learned_at = 0; }
                record.origin = origin;

                size_t idx = facts_.size();
                facts_.push_back(record);
                concept_index_[record.source].push_back(idx);
                concept_index_[record.target].push_back(idx);
                loaded++;
            }
        }

        std::cout << "[NGF GrowthEngine] Restored " << loaded << " facts from disk" << std::endl;
        return true;
    }

    // --- Hot Cache ---
    std::vector<MKHotCacheEntry> getHotConcepts(size_t top_n = 20) const {
        std::vector<MKHotCacheEntry> result;
        for (const auto& [key, entry] : hot_cache_) {
            result.push_back(entry);
        }
        std::sort(result.begin(), result.end(),
                 [](const MKHotCacheEntry& a, const MKHotCacheEntry& b) {
                     return a.access_count > b.access_count;
                 });
        if (result.size() > top_n) result.resize(top_n);
        return result;
    }

    bool isHot(const std::string& concept) const {
        return hot_cache_.find(normalize(concept)) != hot_cache_.end();
    }

    // --- Pruning ---
    size_t pruneStale(uint64_t max_age_seconds = 86400 * 30) {
        uint64_t cutoff = now() - max_age_seconds;
        size_t pruned = 0;

        // Mark stale facts (low confidence + old + never accessed)
        for (auto& fact : facts_) {
            if (fact.learned_at < cutoff &&
                fact.access_count == 0 &&
                fact.confidence < 0.3f) {
                fact.confidence = 0.0f;  // Mark for removal
                pruned++;
            }
        }

        stats_.facts_pruned += pruned;
        std::cout << "[NGF GrowthEngine] Pruned " << pruned << " stale facts" << std::endl;
        return pruned;
    }

    size_t pruneLowConfidence(float threshold = 0.1f) {
        size_t pruned = 0;
        for (auto& fact : facts_) {
            if (fact.confidence < threshold) {
                fact.confidence = 0.0f;
                pruned++;
            }
        }
        stats_.facts_pruned += pruned;
        return pruned;
    }

    // --- Merging duplicates ---
    size_t mergeDuplicates() {
        size_t merged = 0;
        std::unordered_map<std::string, size_t> seen;

        for (size_t i = 0; i < facts_.size(); i++) {
            std::string key = facts_[i].source + "|" + facts_[i].relation + "|" + facts_[i].target;
            auto it = seen.find(key);
            if (it != seen.end()) {
                // Merge: keep higher confidence, average weight
                size_t original = it->second;
                facts_[original].weight = (facts_[original].weight + facts_[i].weight) / 2.0f;
                facts_[original].confidence = std::max(facts_[original].confidence, facts_[i].confidence);
                facts_[original].access_count += facts_[i].access_count;
                facts_[i].confidence = 0.0f;  // Mark duplicate for removal
                merged++;
            } else {
                seen[key] = i;
            }
        }

        stats_.duplicates_merged += merged;
        return merged;
    }

    // --- Querying ---
    std::vector<MKGrowthRecord> getFactsAbout(const std::string& concept) {
        std::string key = normalize(concept);
        std::vector<MKGrowthRecord> result;

        auto it = concept_index_.find(key);
        if (it != concept_index_.end()) {
            for (size_t idx : it->second) {
                if (idx < facts_.size() && facts_[idx].confidence > 0.0f) {
                    facts_[idx].access_count++;
                    facts_[idx].last_accessed = now();
                    result.push_back(facts_[idx]);
                }
            }
        }
        updateHotCache(key);
        return result;
    }

    size_t getFactCount() const { return facts_.size(); }
    size_t getConceptCount() const { return concept_index_.size(); }

    // --- Export to adjacency list format (for activation engine) ---
    std::unordered_map<std::string, std::vector<std::pair<std::string, float>>>
    exportAsGraph() const {
        std::unordered_map<std::string, std::vector<std::pair<std::string, float>>> graph;
        for (const auto& fact : facts_) {
            if (fact.confidence > 0.0f) {
                graph[fact.source].push_back({fact.target, fact.weight * fact.confidence});
            }
        }
        return graph;
    }

    // --- Export edge relations (for path tracer) ---
    std::unordered_map<std::string, std::string> exportEdgeRelations() const {
        std::unordered_map<std::string, std::string> relations;
        for (const auto& fact : facts_) {
            if (fact.confidence > 0.0f) {
                std::string key = fact.source + "|" + fact.target;
                relations[key] = fact.relation;
            }
        }
        return relations;
    }

    // --- Statistics ---
    MKGrowthStats getStats() const {
        MKGrowthStats s = stats_;
        s.hot_cache_size = hot_cache_.size();
        return s;
    }

    void printStats() const {
        std::cout << "[NGF GrowthEngine] === Growth Statistics ===" << std::endl;
        std::cout << "  Total facts: " << facts_.size() << std::endl;
        std::cout << "  Unique concepts: " << concept_index_.size() << std::endl;
        std::cout << "  From conversation: " << stats_.facts_from_conversation << std::endl;
        std::cout << "  From search: " << stats_.facts_from_search << std::endl;
        std::cout << "  From files: " << stats_.facts_from_files << std::endl;
        std::cout << "  Pruned: " << stats_.facts_pruned << std::endl;
        std::cout << "  Conflicts resolved: " << stats_.conflicts_resolved << std::endl;
        std::cout << "  Duplicates merged: " << stats_.duplicates_merged << std::endl;
        std::cout << "  Hot cache size: " << hot_cache_.size() << std::endl;
    }

    // Set persistence path
    void setPersistPath(const std::string& path) { persist_path_ = path; }
    std::string getPersistPath() const { return persist_path_; }
};

#endif // MK_GROWTH_ENGINE_CPP
