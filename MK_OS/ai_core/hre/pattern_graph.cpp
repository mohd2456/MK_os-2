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

    // Index: relation → list of edge indexes (for fast "find all X" queries)
    std::unordered_map<std::string, std::vector<int>> relation_index;
    
    // Index: source+relation → list of edge indexes (for fast specific lookups)
    std::unordered_map<std::string, std::vector<int>> source_relation_index;

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
    // Example: query("cat", "is_a") → returns ["animal", "pet"]
    std::vector<std::string> query(const std::string& source, const std::string& relation) {
        total_queries++;
        std::string src = normalize(source);
        std::string rel = normalize(relation);
        std::vector<std::string> results;

        std::string key = makeKey(src, rel);
        auto it = source_relation_index.find(key);
        if (it != source_relation_index.end()) {
            for (int idx : it->second) {
                results.push_back(edges[idx].target);
            }
            if (!results.empty()) nodes[src].access_count++;
        }
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

    // Load all .mk files from knowledge directory
    void loadAllKnowledge() {
        std::cout << "[PATTERN GRAPH] Loading all knowledge files from: " << knowledge_dir << "/\n";
        // Try loading standard knowledge files
        loadFromFile("core_facts.mk");
        loadFromFile("personal_facts.mk");
        loadFromFile("learned_facts.mk");
        loadFromFile("world_knowledge.mk");
        loadFromFile("rules.mk");
        loadFromFile("coding_knowledge.mk");
        loadFromFile("system_knowledge.mk");
        // Extended knowledge bases
        loadFromFile("science_facts.mk");
        loadFromFile("technology_facts.mk");
        loadFromFile("geography_history.mk");
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

    // Return a const reference to ALL edges in the graph (for bulk indexing)
    const std::vector<MKEdge>& getAllEdges() const { return edges; }

    void printStats() const {
        std::cout << "\n--- [PATTERN GRAPH] ---\n";
        std::cout << "  Nodes: " << nodes.size() << "\n";
        std::cout << "  Edges (facts): " << edges.size() << "\n";
        std::cout << "  Relations: " << relation_index.size() << "\n";
        std::cout << "  Queries served: " << total_queries << "\n";
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
