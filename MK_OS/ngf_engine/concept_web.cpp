#ifndef MK_CONCEPT_WEB_CPP
#define MK_CONCEPT_WEB_CPP

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
#include <functional>
#include <limits>
#include <memory>
#include <chrono>

// ===================================================================================
// MK NEURAL GRAPH FUSION - CONCEPT WEB
// ===================================================================================
// The foundational concept graph for the NGF engine. Stores millions of concept nodes
// connected by typed, weighted relationships. Uses adjacency lists with unordered_maps
// for O(1) lookup by concept name.
//
// Relationship Types:
//   is_a, causes, enables, prevents, part_of, has_property, opposite_of,
//   used_for, made_of, located_in, requires, produces, similar_to,
//   stronger_than, weaker_than, contains, precedes, follows, implies,
//   contradicts, supports, derived_from, example_of, synonym_of
// ===================================================================================

enum class MKNGFRelationType {
    IS_A, CAUSES, ENABLES, PREVENTS, PART_OF, HAS_PROPERTY,
    OPPOSITE_OF, USED_FOR, MADE_OF, LOCATED_IN, REQUIRES,
    PRODUCES, SIMILAR_TO, STRONGER_THAN, WEAKER_THAN, CONTAINS,
    PRECEDES, FOLLOWS, IMPLIES, CONTRADICTS, SUPPORTS,
    DERIVED_FROM, EXAMPLE_OF, SYNONYM_OF, UNKNOWN
};

struct MKNGFEdge {
    std::string target;
    MKNGFRelationType relation;
    float weight;
    float confidence;
    uint64_t timestamp;
    uint32_t access_count;

    MKNGFEdge() : relation(MKNGFRelationType::UNKNOWN), weight(0.5f),
                  confidence(0.8f), timestamp(0), access_count(0) {}

    MKNGFEdge(const std::string& t, MKNGFRelationType r, float w, float c = 0.8f)
        : target(t), relation(r), weight(w), confidence(c), access_count(0) {
        auto now = std::chrono::system_clock::now();
        timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            now.time_since_epoch()).count();
    }
};

struct MKNGFNode {
    std::string name;
    std::string category;
    std::vector<MKNGFEdge> edges;
    float importance;
    uint32_t access_count;
    uint64_t last_accessed;
    bool is_abstract;

    MKNGFNode() : importance(0.5f), access_count(0), last_accessed(0), is_abstract(false) {}
    MKNGFNode(const std::string& n, const std::string& cat = "general")
        : name(n), category(cat), importance(0.5f), access_count(0),
          last_accessed(0), is_abstract(false) {}
};

struct MKNGFPathResult {
    bool found;
    std::vector<std::string> nodes;
    std::vector<MKNGFRelationType> relations;
    std::vector<float> weights;
    float total_weight;
    float avg_confidence;
    int hops;
    MKNGFPathResult() : found(false), total_weight(0.0f), avg_confidence(0.0f), hops(0) {}
};

struct MKNGFRelationQuery {
    std::string source;
    MKNGFRelationType relation;
    std::vector<std::pair<std::string, float>> results;
};

class MKConceptWeb {
private:
    std::unordered_map<std::string, MKNGFNode> nodes_;
    std::unordered_map<std::string, std::vector<std::string>> reverse_index_;
    std::unordered_map<std::string, std::vector<std::string>> category_index_;
    size_t total_edges_;
    size_t total_queries_;
    size_t total_paths_found_;

    MKNGFRelationType stringToRelation(const std::string& s) const {
        static const std::unordered_map<std::string, MKNGFRelationType> map = {
            {"is_a", MKNGFRelationType::IS_A}, {"causes", MKNGFRelationType::CAUSES},
            {"enables", MKNGFRelationType::ENABLES}, {"prevents", MKNGFRelationType::PREVENTS},
            {"part_of", MKNGFRelationType::PART_OF}, {"has_property", MKNGFRelationType::HAS_PROPERTY},
            {"opposite_of", MKNGFRelationType::OPPOSITE_OF}, {"used_for", MKNGFRelationType::USED_FOR},
            {"made_of", MKNGFRelationType::MADE_OF}, {"located_in", MKNGFRelationType::LOCATED_IN},
            {"requires", MKNGFRelationType::REQUIRES}, {"produces", MKNGFRelationType::PRODUCES},
            {"similar_to", MKNGFRelationType::SIMILAR_TO}, {"stronger_than", MKNGFRelationType::STRONGER_THAN},
            {"weaker_than", MKNGFRelationType::WEAKER_THAN}, {"contains", MKNGFRelationType::CONTAINS},
            {"precedes", MKNGFRelationType::PRECEDES}, {"follows", MKNGFRelationType::FOLLOWS},
            {"implies", MKNGFRelationType::IMPLIES}, {"contradicts", MKNGFRelationType::CONTRADICTS},
            {"supports", MKNGFRelationType::SUPPORTS}, {"derived_from", MKNGFRelationType::DERIVED_FROM},
            {"example_of", MKNGFRelationType::EXAMPLE_OF}, {"synonym_of", MKNGFRelationType::SYNONYM_OF}
        };
        auto it = map.find(s);
        return (it != map.end()) ? it->second : MKNGFRelationType::UNKNOWN;
    }

    std::string relationToString(MKNGFRelationType r) const {
        static const std::string names[] = {
            "is_a", "causes", "enables", "prevents", "part_of",
            "has_property", "opposite_of", "used_for", "made_of",
            "located_in", "requires", "produces", "similar_to",
            "stronger_than", "weaker_than", "contains", "precedes",
            "follows", "implies", "contradicts", "supports",
            "derived_from", "example_of", "synonym_of", "unknown"
        };
        int idx = static_cast<int>(r);
        if (idx >= 0 && idx < 25) return names[idx];
        return "unknown";
    }

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

    void touchNode(const std::string& name) {
        auto it = nodes_.find(name);
        if (it != nodes_.end()) {
            it->second.access_count++;
            auto now = std::chrono::system_clock::now();
            it->second.last_accessed = std::chrono::duration_cast<std::chrono::seconds>(
                now.time_since_epoch()).count();
        }
    }

public:
    MKConceptWeb() : total_edges_(0), total_queries_(0), total_paths_found_(0) {
        std::cout << "[NGF ConceptWeb] Initialized concept graph" << std::endl;
    }

    bool addNode(const std::string& name, const std::string& category = "general",
                 float importance = 0.5f, bool is_abstract = false) {
        std::string key = normalize(name);
        if (key.empty()) return false;
        if (nodes_.find(key) != nodes_.end()) {
            nodes_[key].importance = std::max(nodes_[key].importance, importance);
            return false;
        }
        MKNGFNode node(key, category);
        node.importance = importance;
        node.is_abstract = is_abstract;
        nodes_[key] = node;
        category_index_[category].push_back(key);
        return true;
    }

    bool hasNode(const std::string& name) const {
        return nodes_.find(normalize(name)) != nodes_.end();
    }

    const MKNGFNode* getNode(const std::string& name) const {
        std::string key = normalize(name);
        auto it = nodes_.find(key);
        if (it != nodes_.end()) return &(it->second);
        return nullptr;
    }

    size_t nodeCount() const { return nodes_.size(); }
    size_t edgeCount() const { return total_edges_; }

    bool addEdge(const std::string& source, const std::string& target,
                 MKNGFRelationType relation, float weight = 0.5f, float confidence = 0.8f) {
        std::string src = normalize(source);
        std::string tgt = normalize(target);
        if (src.empty() || tgt.empty()) return false;
        if (nodes_.find(src) == nodes_.end()) addNode(src);
        if (nodes_.find(tgt) == nodes_.end()) addNode(tgt);
        for (auto& edge : nodes_[src].edges) {
            if (edge.target == tgt && edge.relation == relation) {
                edge.weight = edge.weight * 0.7f + weight * 0.3f;
                edge.confidence = std::max(edge.confidence, confidence);
                return false;
            }
        }
        nodes_[src].edges.emplace_back(tgt, relation, weight, confidence);
        reverse_index_[tgt].push_back(src);
        total_edges_++;
        return true;
    }

    bool addEdge(const std::string& source, const std::string& target,
                 const std::string& relation, float weight = 0.5f, float confidence = 0.8f) {
        return addEdge(source, target, stringToRelation(relation), weight, confidence);
    }

    std::vector<MKNGFEdge> getEdgesFrom(const std::string& name) {
        std::string key = normalize(name);
        total_queries_++;
        touchNode(key);
        auto it = nodes_.find(key);
        if (it != nodes_.end()) return it->second.edges;
        return {};
    }

    std::vector<std::string> getEdgesTo(const std::string& name) {
        std::string key = normalize(name);
        total_queries_++;
        auto it = reverse_index_.find(key);
        if (it != reverse_index_.end()) return it->second;
        return {};
    }

    MKNGFRelationQuery queryRelation(const std::string& source, MKNGFRelationType relation) {
        MKNGFRelationQuery result;
        result.source = normalize(source);
        result.relation = relation;
        total_queries_++;
        auto it = nodes_.find(result.source);
        if (it != nodes_.end()) {
            touchNode(result.source);
            for (const auto& edge : it->second.edges) {
                if (edge.relation == relation)
                    result.results.push_back({edge.target, edge.weight});
            }
            std::sort(result.results.begin(), result.results.end(),
                     [](const auto& a, const auto& b) { return a.second > b.second; });
        }
        return result;
    }

    MKNGFRelationQuery queryRelation(const std::string& source, const std::string& relation) {
        return queryRelation(source, stringToRelation(relation));
    }

    std::vector<std::string> getByCategory(const std::string& category) const {
        auto it = category_index_.find(category);
        if (it != category_index_.end()) return it->second;
        return {};
    }

    std::vector<std::string> getNeighborhood(const std::string& name, int max_hops = 2) {
        std::string key = normalize(name);
        std::unordered_set<std::string> visited;
        std::queue<std::pair<std::string, int>> frontier;
        std::vector<std::string> result;
        frontier.push({key, 0});
        visited.insert(key);
        while (!frontier.empty()) {
            auto [current, depth] = frontier.front();
            frontier.pop();
            if (depth > 0) result.push_back(current);
            if (depth >= max_hops) continue;
            auto it = nodes_.find(current);
            if (it != nodes_.end()) {
                for (const auto& edge : it->second.edges) {
                    if (visited.find(edge.target) == visited.end()) {
                        visited.insert(edge.target);
                        frontier.push({edge.target, depth + 1});
                    }
                }
            }
        }
        return result;
    }

    MKNGFPathResult findPathBFS(const std::string& source, const std::string& target,
                                int max_depth = 10) {
        MKNGFPathResult result;
        std::string src = normalize(source);
        std::string tgt = normalize(target);
        total_queries_++;
        if (src == tgt) { result.found = true; result.nodes.push_back(src); return result; }
        if (nodes_.find(src) == nodes_.end() || nodes_.find(tgt) == nodes_.end()) return result;
        std::unordered_map<std::string, std::string> parent;
        std::unordered_map<std::string, MKNGFRelationType> parent_rel;
        std::unordered_map<std::string, float> parent_wt;
        std::queue<std::pair<std::string, int>> frontier;
        frontier.push({src, 0});
        parent[src] = "";
        while (!frontier.empty()) {
            auto [current, depth] = frontier.front();
            frontier.pop();
            if (depth >= max_depth) continue;
            auto it = nodes_.find(current);
            if (it == nodes_.end()) continue;
            for (const auto& edge : it->second.edges) {
                if (parent.find(edge.target) != parent.end()) continue;
                parent[edge.target] = current;
                parent_rel[edge.target] = edge.relation;
                parent_wt[edge.target] = edge.weight;
                if (edge.target == tgt) {
                    result.found = true;
                    std::string node = tgt;
                    while (!node.empty()) {
                        result.nodes.push_back(node);
                        if (parent_rel.find(node) != parent_rel.end()) {
                            result.relations.push_back(parent_rel[node]);
                            result.weights.push_back(parent_wt[node]);
                            result.total_weight += parent_wt[node];
                        }
                        node = parent[node];
                    }
                    std::reverse(result.nodes.begin(), result.nodes.end());
                    std::reverse(result.relations.begin(), result.relations.end());
                    std::reverse(result.weights.begin(), result.weights.end());
                    result.hops = (int)result.nodes.size() - 1;
                    if (!result.weights.empty()) {
                        float sum = 0; for (float w : result.weights) sum += w;
                        result.avg_confidence = sum / result.weights.size();
                    }
                    total_paths_found_++;
                    return result;
                }
                frontier.push({edge.target, depth + 1});
            }
        }
        return result;
    }

    MKNGFPathResult findPathWeighted(const std::string& source, const std::string& target,
                                     int max_depth = 15) {
        MKNGFPathResult result;
        std::string src = normalize(source);
        std::string tgt = normalize(target);
        total_queries_++;
        if (nodes_.find(src) == nodes_.end() || nodes_.find(tgt) == nodes_.end()) return result;
        using PQEntry = std::pair<float, std::string>;
        std::priority_queue<PQEntry, std::vector<PQEntry>, std::greater<PQEntry>> pq;
        std::unordered_map<std::string, float> dist;
        std::unordered_map<std::string, std::string> parent;
        std::unordered_map<std::string, MKNGFRelationType> parent_rel;
        std::unordered_map<std::string, float> parent_wt;
        std::unordered_map<std::string, int> depth_map;
        dist[src] = 0.0f; parent[src] = ""; depth_map[src] = 0;
        pq.push({0.0f, src});
        while (!pq.empty()) {
            auto [cost, current] = pq.top(); pq.pop();
            if (current == tgt) break;
            if (cost > dist[current]) continue;
            if (depth_map[current] >= max_depth) continue;
            auto it = nodes_.find(current);
            if (it == nodes_.end()) continue;
            for (const auto& edge : it->second.edges) {
                float edge_cost = 1.0f - edge.weight * edge.confidence;
                float new_cost = cost + edge_cost;
                if (dist.find(edge.target) == dist.end() || new_cost < dist[edge.target]) {
                    dist[edge.target] = new_cost;
                    parent[edge.target] = current;
                    parent_rel[edge.target] = edge.relation;
                    parent_wt[edge.target] = edge.weight;
                    depth_map[edge.target] = depth_map[current] + 1;
                    pq.push({new_cost, edge.target});
                }
            }
        }
        if (parent.find(tgt) != parent.end()) {
            result.found = true;
            std::string node = tgt;
            while (!node.empty()) {
                result.nodes.push_back(node);
                if (parent_rel.find(node) != parent_rel.end()) {
                    result.relations.push_back(parent_rel[node]);
                    result.weights.push_back(parent_wt[node]);
                    result.total_weight += parent_wt[node];
                }
                node = parent[node];
            }
            std::reverse(result.nodes.begin(), result.nodes.end());
            std::reverse(result.relations.begin(), result.relations.end());
            std::reverse(result.weights.begin(), result.weights.end());
            result.hops = (int)result.nodes.size() - 1;
            if (!result.weights.empty()) {
                float sum = 0; for (float w : result.weights) sum += w;
                result.avg_confidence = sum / result.weights.size();
            }
            total_paths_found_++;
        }
        return result;
    }

    size_t loadFromFile(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cout << "[NGF ConceptWeb] Failed to open: " << filepath << std::endl;
            return 0;
        }
        size_t loaded = 0;
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            std::istringstream iss(line);
            std::string source, relation, target_str, weight_str;
            if (std::getline(iss, source, '|') && std::getline(iss, relation, '|') &&
                std::getline(iss, target_str, '|') && std::getline(iss, weight_str, '|')) {
                float weight = 0.5f;
                try { weight = std::stof(weight_str); } catch (...) {}
                addEdge(source, target_str, relation, weight);
                loaded++;
            }
        }
        std::cout << "[NGF ConceptWeb] Loaded " << loaded << " edges from " << filepath << std::endl;
        return loaded;
    }

    size_t saveToFile(const std::string& filepath) const {
        std::ofstream file(filepath);
        if (!file.is_open()) return 0;
        file << "# MK NGF ConceptWeb Export\n# Format: source|relation|target|weight\n";
        size_t saved = 0;
        for (const auto& [name, node] : nodes_) {
            for (const auto& edge : node.edges) {
                file << name << "|" << relationToString(edge.relation)
                     << "|" << edge.target << "|" << edge.weight << "\n";
                saved++;
            }
        }
        return saved;
    }

    std::vector<std::pair<std::string, size_t>> getHubs(size_t top_n = 10) const {
        std::vector<std::pair<std::string, size_t>> hub_list;
        for (const auto& [name, node] : nodes_)
            hub_list.push_back({name, node.edges.size()});
        std::sort(hub_list.begin(), hub_list.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        if (hub_list.size() > top_n) hub_list.resize(top_n);
        return hub_list;
    }

    std::vector<std::pair<std::string, uint32_t>> getHotConcepts(size_t top_n = 10) const {
        std::vector<std::pair<std::string, uint32_t>> hot_list;
        for (const auto& [name, node] : nodes_) {
            if (node.access_count > 0) hot_list.push_back({name, node.access_count});
        }
        std::sort(hot_list.begin(), hot_list.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        if (hot_list.size() > top_n) hot_list.resize(top_n);
        return hot_list;
    }

    float getDensity() const {
        if (nodes_.size() < 2) return 0.0f;
        return (float)total_edges_ / (float)(nodes_.size() * (nodes_.size() - 1));
    }

    std::unordered_map<std::string, size_t> getRelationDistribution() const {
        std::unordered_map<std::string, size_t> dist;
        for (const auto& [name, node] : nodes_)
            for (const auto& edge : node.edges)
                dist[relationToString(edge.relation)]++;
        return dist;
    }

    void printStats() const {
        std::cout << "[NGF ConceptWeb] === Statistics ===" << std::endl;
        std::cout << "  Nodes: " << nodes_.size() << std::endl;
        std::cout << "  Edges: " << total_edges_ << std::endl;
        std::cout << "  Categories: " << category_index_.size() << std::endl;
        std::cout << "  Density: " << getDensity() << std::endl;
        std::cout << "  Queries: " << total_queries_ << std::endl;
        std::cout << "  Paths found: " << total_paths_found_ << std::endl;
    }

    std::vector<std::string> getAllNodes() const {
        std::vector<std::string> result;
        result.reserve(nodes_.size());
        for (const auto& [name, node] : nodes_) result.push_back(name);
        return result;
    }

    std::string getRelationName(MKNGFRelationType r) const { return relationToString(r); }
    MKNGFRelationType parseRelation(const std::string& s) const { return stringToRelation(s); }

    void clear() {
        nodes_.clear(); reverse_index_.clear(); category_index_.clear();
        total_edges_ = 0; total_queries_ = 0; total_paths_found_ = 0;
    }
};

#endif // MK_CONCEPT_WEB_CPP
