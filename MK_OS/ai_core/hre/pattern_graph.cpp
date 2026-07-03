#ifndef MK_PATTERN_GRAPH_CPP
#define MK_PATTERN_GRAPH_CPP

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <set>
#include <list>
#include <dirent.h>

// ===================================================================================
// MK HYBRID REASONING ENGINE — LAYER 1: PATTERN GRAPH
// ===================================================================================
// The core knowledge representation. NOT a neural network.
// A directed graph where:
//   - Nodes = concepts (words, entities, ideas)
//   - Edges = relationships between concepts
// 
// How it works:
//   "cat" --[is_a]--> "animal"
//   "animal" --[has]--> "legs"
//   Query: "does cat have legs?" → walk graph: cat→animal→legs → YES
//
// All knowledge persists to flat files. New facts = new lines in files.
// Zero training. Zero matrix math. Pure graph traversal.
// ===================================================================================

// A single relationship edge in the graph
struct MKEdge {
    std::string source;         // From concept
    std::string relation;       // Relationship type (is_a, has, can, lives_in, etc.)
    std::string target;         // To concept
    float weight;               // Confidence/strength (0.0 to 1.0)
    std::string source_file;    // Which knowledge file this came from
    std::time_t learned_at;     // When this edge was added
};

// A concept node with all its connections
struct MKNode {
    std::string name;
    std::vector<int> outgoing_edges;    // Indexes into edge list
    std::vector<int> incoming_edges;    // Indexes into edge list
    std::string domain;                  // Category: "general", "personal", "technical", etc.
    int access_count;                    // How often this node is queried
};

// Result of a graph traversal/query
struct MKGraphResult {
    bool found;
    std::string answer;
    std::vector<MKEdge> path;           // The edges traversed to reach answer
    float confidence;
    int hops;                           // How many edges we walked
};

class MKPatternGraph {
private:
    std::unordered_map<std::string, MKNode> nodes;
    std::vector<MKEdge> edges;
    std::string knowledge_dir;
    int total_facts;
    int total_queries;
    int cache_hits;

    // Index: relation -> list of edge indexes (for fast "find all X" queries)
    std::unordered_map<std::string, std::vector<int>> relation_index;
    
    // Index: source+relation -> list of edge indexes (for fast specific lookups)
    std::unordered_map<std::string, std::vector<int>> source_relation_index;

    // LRU Query Cache: maps query key -> cached results
    static const int LRU_CAPACITY = 256;
    std::list<std::pair<std::string, std::vector<std::string>>> lru_list;
    std::unordered_map<std::string, decltype(lru_list)::iterator> lru_map;

    // LRU cache helpers
    void lruPut(const std::string& key, const std::vector<std::string>& value) {
        auto it = lru_map.find(key);
        if (it != lru_map.end()) {
            lru_list.erase(it->second);
            lru_map.erase(it);
        }
        lru_list.push_front({key, value});
        lru_map[key] = lru_list.begin();
        if ((int)lru_map.size() > LRU_CAPACITY) {
            auto last = lru_list.end();
            --last;
            lru_map.erase(last->first);
            lru_list.pop_back();
        }
    }

    bool lruGet(const std::string& key, std::vector<std::string>& out) {
        auto it = lru_map.find(key);
        if (it == lru_map.end()) return false;
        lru_list.splice(lru_list.begin(), lru_list, it->second);
        out = it->second->second;
        cache_hits++;
        return true;
    }

    void lruInvalidate() {
        lru_list.clear();
        lru_map.clear();
    }

    // Normalize concept names (lowercase, trim whitespace)
    std::string normalize(const std::string& concept) {
        std::string result = concept;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        // Trim
        size_t start = result.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        size_t end = result.find_last_not_of(" \t\r\n");
        return result.substr(start, end - start + 1);
    }

    // Build a lookup key for the source+relation index
    std::string makeKey(const std::string& source, const std::string& relation) {
        return source + "|" + relation;
    }

    // Ensure a node exists in the graph
    void ensureNode(const std::string& name) {
        if (nodes.find(name) == nodes.end()) {
            MKNode node;
            node.name = name;
            node.domain = "general";
            node.access_count = 0;
            nodes[name] = node;
        }
    }

public:
    MKPatternGraph(const std::string& knowledgeDirectory = "knowledge_files") 
        : knowledge_dir(knowledgeDirectory), total_facts(0), total_queries(0), cache_hits(0) {
        std::cout << "[PATTERN GRAPH] Knowledge graph engine initialized.\n";
    }

    // ─────────────────────────────────────────
    //  CORE: ADD FACTS
    // ─────────────────────────────────────────

    // Add a single fact (edge) to the graph
    void addFact(const std::string& source, const std::string& relation, 
                 const std::string& target, float weight = 1.0f,
                 const std::string& file = "runtime") {
        
        std::string src = normalize(source);
        std::string rel = normalize(relation);
        std::string tgt = normalize(target);
        
        if (src.empty() || rel.empty() || tgt.empty()) return;

        // Check for duplicate
        std::string key = makeKey(src, rel);
        auto it = source_relation_index.find(key);
        if (it != source_relation_index.end()) {
            for (int idx : it->second) {
                if (edges[idx].target == tgt) {
                    // Already exists — just reinforce weight
                    edges[idx].weight = std::min(1.0f, edges[idx].weight + 0.1f);
                    return;
                }
            }
        }

        // Create edge
        MKEdge edge;
        edge.source = src;
        edge.relation = rel;
        edge.target = tgt;
        edge.weight = weight;
        edge.source_file = file;
        edge.learned_at = std::time(nullptr);

        int edgeIdx = edges.size();
        edges.push_back(edge);

        // Create/update nodes
        ensureNode(src);
        ensureNode(tgt);
        nodes[src].outgoing_edges.push_back(edgeIdx);
        nodes[tgt].incoming_edges.push_back(edgeIdx);

        // Update indexes
        relation_index[rel].push_back(edgeIdx);
        source_relation_index[key].push_back(edgeIdx);

        // Invalidate LRU cache on new knowledge
        lruInvalidate();

        total_facts++;
    }

    // Add multiple facts at once from text format: "source|relation|target"
    void addFactsFromText(const std::string& text) {
        std::stringstream ss(text);
        std::string line;
        while (std::getline(ss, line)) {
            if (line.empty() || line[0] == '#') continue;
            
            std::stringstream ls(line);
            std::string src, rel, tgt;
            if (std::getline(ls, src, '|') && std::getline(ls, rel, '|') && std::getline(ls, tgt, '|')) {
                addFact(src, rel, tgt);
            }
        }
    }

    // ─────────────────────────────────────────
    //  CORE: QUERY THE GRAPH
    // ─────────────────────────────────────────

    // Direct lookup: "what does [source] [relation]?"
    // Example: query("cat", "is_a") -> returns ["animal", "pet"]
    std::vector<std::string> query(const std::string& source, const std::string& relation) {
        total_queries++;
        std::string src = normalize(source);
        std::string rel = normalize(relation);
        std::vector<std::string> results;

        // Check LRU cache first
        std::string cacheKey = src + "|" + rel;
        if (lruGet(cacheKey, results)) {
            if (!results.empty()) nodes[src].access_count++;
            return results;
        }

        std::string key = makeKey(src, rel);
        auto it = source_relation_index.find(key);
        if (it != source_relation_index.end()) {
            for (int idx : it->second) {
                results.push_back(edges[idx].target);
            }
            if (!results.empty()) nodes[src].access_count++;
        }

        // Store in LRU cache
        lruPut(cacheKey, results);
        return results;
    }

    // Reverse lookup: "what [relation] [target]?"
    // Example: reverseQuery("is_a", "animal") → returns ["cat", "dog", "bird"]
    std::vector<std::string> reverseQuery(const std::string& relation, const std::string& target) {
        total_queries++;
        std::string rel = normalize(relation);
        std::string tgt = normalize(target);
        std::vector<std::string> results;

        auto it = relation_index.find(rel);
        if (it != relation_index.end()) {
            for (int idx : it->second) {
                if (edges[idx].target == tgt) {
                    results.push_back(edges[idx].source);
                }
            }
        }
        return results;
    }

    // Walk the graph: follow edges up to N hops to find connections
    // Example: pathQuery("cat", "has") → cat--is_a-->animal--has-->legs → "legs"
    MKGraphResult pathQuery(const std::string& source, const std::string& targetRelation, 
                            int maxHops = 3) {
        total_queries++;
        MKGraphResult result;
        result.found = false;
        result.confidence = 0.0f;
        result.hops = 0;

        std::string src = normalize(source);
        std::string tgtRel = normalize(targetRelation);

        // First try direct lookup
        auto direct = query(src, tgtRel);
        if (!direct.empty()) {
            result.found = true;
            result.answer = direct[0];
            result.confidence = 1.0f;
            result.hops = 1;
            return result;
        }

        // BFS walk through the graph following inheritance chains
        std::set<std::string> visited;
        std::vector<std::pair<std::string, int>> frontier;  // (node, depth)
        frontier.push_back({src, 0});
        visited.insert(src);

        while (!frontier.empty()) {
            auto [current, depth] = frontier.front();
            frontier.erase(frontier.begin());

            if (depth >= maxHops) continue;

            // Get all outgoing edges from current node
            auto nodeIt = nodes.find(current);
            if (nodeIt == nodes.end()) continue;

            for (int edgeIdx : nodeIt->second.outgoing_edges) {
                const MKEdge& edge = edges[edgeIdx];

                // If this edge's target has the relation we want, we found it!
                auto targetResults = query(edge.target, tgtRel);
                if (!targetResults.empty()) {
                    result.found = true;
                    result.answer = targetResults[0];
                    result.confidence = 1.0f / (depth + 2);  // Confidence decreases with hops
                    result.hops = depth + 2;
                    result.path.push_back(edge);
                    return result;
                }

                // Otherwise, if this is an inheritance relation, keep walking
                if (edge.relation == "is_a" || edge.relation == "type_of" || 
                    edge.relation == "kind_of" || edge.relation == "part_of") {
                    if (visited.find(edge.target) == visited.end()) {
                        visited.insert(edge.target);
                        frontier.push_back({edge.target, depth + 1});
                        result.path.push_back(edge);
                    }
                }
            }
        }

        return result;
    }

    // Find everything known about a concept
    std::vector<MKEdge> getAll(const std::string& concept) {
        std::string name = normalize(concept);
        std::vector<MKEdge> results;

        auto it = nodes.find(name);
        if (it == nodes.end()) return results;

        for (int idx : it->second.outgoing_edges) {
            results.push_back(edges[idx]);
        }
        return results;
    }

    // Check if a specific fact exists
    bool hasFact(const std::string& source, const std::string& relation, const std::string& target) {
        std::string key = makeKey(normalize(source), normalize(relation));
        std::string tgt = normalize(target);
        
        auto it = source_relation_index.find(key);
        if (it != source_relation_index.end()) {
            for (int idx : it->second) {
                if (edges[idx].target == tgt) return true;
            }
        }
        return false;
    }

    // Remove a specific edge (for corrections)
    void removeEdge(const std::string& source, const std::string& relation, const std::string& target) {
        std::string src = normalize(source);
        std::string rel = normalize(relation);
        std::string tgt = normalize(target);
        std::string key = makeKey(src, rel);

        auto it = source_relation_index.find(key);
        if (it != source_relation_index.end()) {
            for (auto idxIt = it->second.begin(); idxIt != it->second.end(); ++idxIt) {
                if (edges[*idxIt].target == tgt) {
                    edges[*idxIt].weight = 0.0f; // Mark as removed (soft delete)
                    it->second.erase(idxIt);
                    break;
                }
            }
        }
    }

    // ─────────────────────────────────────────
    //  PERSISTENCE: SAVE/LOAD KNOWLEDGE FILES
    // ─────────────────────────────────────────

    // Save all knowledge to a file (append new facts only)
    void saveToFile(const std::string& filename) {
        std::string path = knowledge_dir + "/" + filename;
        std::ofstream out(path);
        if (!out.is_open()) {
            std::cerr << "[PATTERN GRAPH] Cannot write to: " << path << "\n";
            return;
        }

        out << "# MK Knowledge File — Auto-generated\n";
        out << "# Format: source|relation|target|weight\n";
        out << "# Total facts: " << edges.size() << "\n\n";

        for (const auto& edge : edges) {
            out << edge.source << "|" << edge.relation << "|" 
                << edge.target << "|" << edge.weight << "\n";
        }
        out.close();
        std::cout << "[PATTERN GRAPH] Saved " << edges.size() << " facts to: " << path << "\n";
    }

    // Load knowledge from a file
    void loadFromFile(const std::string& filename) {
        std::string path = knowledge_dir + "/" + filename;
        std::ifstream in(path);
        if (!in.is_open()) {
            std::cerr << "[PATTERN GRAPH] Cannot read: " << path << "\n";
            return;
        }

        std::string line;
        int loaded = 0;
        while (std::getline(in, line)) {
            if (line.empty() || line[0] == '#') continue;

            std::stringstream ss(line);
            std::string src, rel, tgt, weightStr;
            if (std::getline(ss, src, '|') && std::getline(ss, rel, '|') && 
                std::getline(ss, tgt, '|')) {
                
                float w = 1.0f;
                if (std::getline(ss, weightStr, '|')) {
                    try { w = std::stof(weightStr); } catch (...) {}
                }
                addFact(src, rel, tgt, w, filename);
                loaded++;
            }
        }
        in.close();
        std::cout << "[PATTERN GRAPH] Loaded " << loaded << " facts from: " << path << "\n";
    }

    // Load all .mk files from knowledge directory (dynamic scan)
    void loadAllKnowledge() {
        std::cout << "[PATTERN GRAPH] Scanning all .mk files from: " << knowledge_dir << "/\n";
        
        // Dynamically scan directory for all .mk files
        std::vector<std::string> mk_files;
        
        DIR* dir = opendir(knowledge_dir.c_str());
        if (dir) {
            struct dirent* entry;
            while ((entry = readdir(dir)) != nullptr) {
                std::string fname(entry->d_name);
                // Only load .mk files
                if (fname.size() > 3 && fname.substr(fname.size() - 3) == ".mk") {
                    mk_files.push_back(fname);
                }
            }
            closedir(dir);
            // Sort for deterministic loading order
            std::sort(mk_files.begin(), mk_files.end());
        } else {
            std::cerr << "[PATTERN GRAPH] Cannot open directory: " << knowledge_dir << "\n";
            // Fallback to known files
            mk_files = {"core_facts.mk", "personal_facts.mk", "learned_facts.mk",
                         "world_knowledge.mk", "rules.mk", "coding_knowledge.mk",
                         "system_knowledge.mk", "science_facts.mk", "technology_facts.mk",
                         "geography_history.mk"};
        }
        
        int total_loaded = 0;
        for (const auto& file : mk_files) {
            loadFromFile(file);
            total_loaded++;
        }
        std::cout << "[PATTERN GRAPH] Loaded " << total_loaded << " knowledge files, "
                  << edges.size() << " total facts.\n";
    }

    // Append a single new fact to the learned_facts file (for runtime learning)
    void persistNewFact(const std::string& source, const std::string& relation, 
                        const std::string& target, float weight = 1.0f) {
        addFact(source, relation, target, weight, "learned_facts.mk");
        
        std::string path = knowledge_dir + "/learned_facts.mk";
        std::ofstream out(path, std::ios::app);
        if (out.is_open()) {
            out << normalize(source) << "|" << normalize(relation) << "|" 
                << normalize(target) << "|" << weight << "\n";
            out.close();
        }
    }

    // ─────────────────────────────────────────
    //  STATS & DEBUG
    // ─────────────────────────────────────────

    int nodeCount() const { return nodes.size(); }
    int edgeCount() const { return edges.size(); }
    int queryCount() const { return total_queries; }
    int cacheHitCount() const { return cache_hits; }

    // Fuzzy string matching using Levenshtein distance
    static int levenshtein(const std::string& a, const std::string& b) {
        int m = a.size(), n = b.size();
        std::vector<int> prev(n + 1), curr(n + 1);
        for (int j = 0; j <= n; j++) prev[j] = j;
        for (int i = 1; i <= m; i++) {
            curr[0] = i;
            for (int j = 1; j <= n; j++) {
                int cost = (a[i-1] == b[j-1]) ? 0 : 1;
                curr[j] = std::min({prev[j] + 1, curr[j-1] + 1, prev[j-1] + cost});
            }
            std::swap(prev, curr);
        }
        return prev[n];
    }

    // Find nodes matching a query with typo tolerance (edit distance <= threshold)
    std::vector<std::string> fuzzyMatch(const std::string& concept, int threshold = 2) {
        std::string name = normalize(concept);
        std::vector<std::string> matches;
        for (const auto& pair : nodes) {
            int dist = levenshtein(name, pair.first);
            if (dist <= threshold && dist > 0) {
                matches.push_back(pair.first);
            }
        }
        // Sort by distance (closest first)
        std::sort(matches.begin(), matches.end(), [&](const std::string& a, const std::string& b) {
            return levenshtein(name, a) < levenshtein(name, b);
        });
        return matches;
    }

    // Return a const reference to ALL edges in the graph (for bulk indexing)
    const std::vector<MKEdge>& getAllEdges() const { return edges; }

    void printStats() const {
        std::cout << "\n--- [PATTERN GRAPH] ---\n";
        std::cout << "  Nodes: " << nodes.size() << "\n";
        std::cout << "  Edges (facts): " << edges.size() << "\n";
        std::cout << "  Relations: " << relation_index.size() << "\n";
        std::cout << "  Queries served: " << total_queries << "\n";
        std::cout << "  Cache hits: " << cache_hits << "\n";
        std::cout << "-----------------------\n";
    }

    // Dump all knowledge (for debugging)
    void dump() const {
        std::cout << "\n=== FULL KNOWLEDGE DUMP ===\n";
        for (const auto& edge : edges) {
            std::cout << "  " << edge.source << " --[" << edge.relation << "]--> " 
                      << edge.target << " (w=" << edge.weight << ")\n";
        }
        std::cout << "=== END DUMP ===\n";
    }
};

#endif // MK_PATTERN_GRAPH_CPP
