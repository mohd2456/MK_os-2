#ifndef MK_PATH_TRACER_CPP
#define MK_PATH_TRACER_CPP

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <stack>
#include <algorithm>
#include <cmath>
#include <limits>
#include <functional>

// ===================================================================================
// MK NEURAL GRAPH FUSION — PATH TRACER
// ===================================================================================
// Multi-algorithm path tracing through the knowledge graph.
// Finds optimal connections between concepts using:
//   1. Breadth-First Search (BFS) - shortest hop path
//   2. Weighted Shortest Path (Dijkstra-style) - highest confidence path
//   3. Depth-First Search (DFS) - all paths enumeration
//   4. Cycle Detection - finds feedback loops in knowledge
//
// Used by NGF Engine to trace reasoning paths between activated nodes.
// ===================================================================================

// A single hop in a traced path
struct MKPathHop {
    std::string from_node;
    std::string to_node;
    std::string relation;
    float edge_weight;
    int hop_number;
};

// A complete traced path between two concepts
struct MKTracedPath {
    std::string source;
    std::string target;
    std::vector<MKPathHop> hops;
    float total_weight;         // Product of edge weights (confidence)
    float path_score;           // Computed relevance score
    int length;                 // Number of hops
    std::string algorithm_used; // Which algorithm found this path
};

// Result of a path tracing operation
struct MKTraceResult {
    bool found;
    std::vector<MKTracedPath> paths;    // All paths found (ranked by score)
    int nodes_explored;
    int paths_found;
    std::string best_summary;           // Human-readable summary of best path
};

// Cycle information
struct MKCycleInfo {
    std::vector<std::string> nodes;     // Nodes in the cycle
    std::vector<std::string> relations; // Relations forming the cycle
    int length;
    float avg_weight;
};

// Internal edge for path tracer
struct MKTracerEdge {
    std::string source;
    std::string target;
    std::string relation;
    float weight;
};

class MKPathTracer {
private:
    std::unordered_map<std::string, std::vector<int>> adjacency;  // node -> edge indices
    std::unordered_map<std::string, std::vector<int>> reverse_adj; // node -> incoming edge indices
    std::vector<MKTracerEdge> edges;
    int max_path_length;
    int max_paths;
    int total_traces;
    int total_paths_found;

    // Reconstruct path from BFS parent map
    MKTracedPath reconstructBFSPath(const std::string& source, const std::string& target,
                                     const std::unordered_map<std::string, std::string>& parent,
                                     const std::unordered_map<std::string, int>& parent_edge) {
        MKTracedPath path;
        path.source = source;
        path.target = target;
        path.total_weight = 1.0f;
        path.algorithm_used = "BFS";

        std::vector<MKPathHop> reversed_hops;
        std::string current = target;
        int hop_num = 0;

        while (current != source) {
            auto p_it = parent.find(current);
            if (p_it == parent.end()) break;
            std::string prev = p_it->second;

            auto e_it = parent_edge.find(current);
            if (e_it != parent_edge.end()) {
                const MKTracerEdge& edge = edges[e_it->second];
                MKPathHop hop;
                hop.from_node = prev;
                hop.to_node = current;
                hop.relation = edge.relation;
                hop.edge_weight = edge.weight;
                hop.hop_number = hop_num++;
                reversed_hops.push_back(hop);
                path.total_weight *= edge.weight;
            }
            current = prev;
        }

        std::reverse(reversed_hops.begin(), reversed_hops.end());
        for (int i = 0; i < static_cast<int>(reversed_hops.size()); i++) {
            reversed_hops[i].hop_number = i + 1;
        }
        path.hops = reversed_hops;
        path.length = static_cast<int>(path.hops.size());
        path.path_score = path.total_weight / std::max(1, path.length);
        return path;
    }

public:
    MKPathTracer(int maxLength = 10, int maxPaths = 20)
        : max_path_length(maxLength), max_paths(maxPaths), total_traces(0), total_paths_found(0) {
        std::cout << "[PATH TRACER] Graph path tracing engine initialized.\n";
        std::cout << "  Max path length: " << max_path_length << ", Max paths: " << max_paths << "\n";
    }

    // ─────────────────────────────────────────
    //  GRAPH BUILDING
    // ─────────────────────────────────────────

    void addEdge(const std::string& source, const std::string& relation,
                 const std::string& target, float weight = 1.0f) {
        MKTracerEdge edge;
        edge.source = source;
        edge.target = target;
        edge.relation = relation;
        edge.weight = std::max(0.01f, std::min(1.0f, weight));

        int idx = static_cast<int>(edges.size());
        edges.push_back(edge);
        adjacency[source].push_back(idx);
        reverse_adj[target].push_back(idx);
    }

    void clear() {
        edges.clear();
        adjacency.clear();
        reverse_adj.clear();
    }

    // ─────────────────────────────────────────
    //  BFS: Shortest hop path
    // ─────────────────────────────────────────

    MKTraceResult traceBFS(const std::string& source, const std::string& target) {
        total_traces++;
        MKTraceResult result;
        result.found = false;
        result.nodes_explored = 0;
        result.paths_found = 0;

        if (source == target) {
            result.found = true;
            result.best_summary = source + " is the same as " + target;
            return result;
        }

        std::queue<std::string> frontier;
        std::unordered_map<std::string, std::string> parent;
        std::unordered_map<std::string, int> parent_edge;
        std::unordered_set<std::string> visited;

        frontier.push(source);
        visited.insert(source);

        while (!frontier.empty()) {
            std::string current = frontier.front();
            frontier.pop();
            result.nodes_explored++;

            auto adj_it = adjacency.find(current);
            if (adj_it == adjacency.end()) continue;

            for (int edge_idx : adj_it->second) {
                const MKTracerEdge& edge = edges[edge_idx];
                if (visited.count(edge.target)) continue;

                visited.insert(edge.target);
                parent[edge.target] = current;
                parent_edge[edge.target] = edge_idx;

                if (edge.target == target) {
                    MKTracedPath path = reconstructBFSPath(source, target, parent, parent_edge);
                    result.paths.push_back(path);
                    result.found = true;
                    result.paths_found = 1;
                    total_paths_found++;

                    // Build summary
                    result.best_summary = source;
                    for (const auto& hop : path.hops) {
                        result.best_summary += " -[" + hop.relation + "]-> " + hop.to_node;
                    }
                    return result;
                }

                // Limit search depth
                int depth = 0;
                std::string trace = edge.target;
                while (parent.count(trace)) {
                    trace = parent[trace];
                    depth++;
                }
                if (depth < max_path_length) {
                    frontier.push(edge.target);
                }
            }
        }

        result.best_summary = "No path found from " + source + " to " + target;
        return result;
    }

    // ─────────────────────────────────────────
    //  WEIGHTED SHORTEST PATH (Dijkstra-style)
    // ─────────────────────────────────────────

    MKTraceResult traceWeighted(const std::string& source, const std::string& target) {
        total_traces++;
        MKTraceResult result;
        result.found = false;
        result.nodes_explored = 0;
        result.paths_found = 0;

        if (source == target) {
            result.found = true;
            result.best_summary = source + " is the same as " + target;
            return result;
        }

        // Use -log(weight) as distance so higher weights = shorter distance
        // Priority queue: (distance, node)
        using PQEntry = std::pair<float, std::string>;
        std::priority_queue<PQEntry, std::vector<PQEntry>, std::greater<PQEntry>> pq;

        std::unordered_map<std::string, float> dist;
        std::unordered_map<std::string, std::string> parent;
        std::unordered_map<std::string, int> parent_edge;

        dist[source] = 0.0f;
        pq.push({0.0f, source});

        while (!pq.empty()) {
            auto [d, current] = pq.top();
            pq.pop();
            result.nodes_explored++;

            if (current == target) {
                // Reconstruct path
                MKTracedPath path;
                path.source = source;
                path.target = target;
                path.total_weight = 1.0f;
                path.algorithm_used = "WeightedShortest";

                std::vector<MKPathHop> reversed_hops;
                std::string node = target;
                while (node != source) {
                    auto p_it = parent.find(node);
                    if (p_it == parent.end()) break;
                    auto e_it = parent_edge.find(node);
                    if (e_it != parent_edge.end()) {
                        const MKTracerEdge& edge = edges[e_it->second];
                        MKPathHop hop;
                        hop.from_node = p_it->second;
                        hop.to_node = node;
                        hop.relation = edge.relation;
                        hop.edge_weight = edge.weight;
                        reversed_hops.push_back(hop);
                        path.total_weight *= edge.weight;
                    }
                    node = p_it->second;
                }

                std::reverse(reversed_hops.begin(), reversed_hops.end());
                for (int i = 0; i < static_cast<int>(reversed_hops.size()); i++) {
                    reversed_hops[i].hop_number = i + 1;
                }
                path.hops = reversed_hops;
                path.length = static_cast<int>(path.hops.size());
                path.path_score = path.total_weight;

                result.paths.push_back(path);
                result.found = true;
                result.paths_found = 1;
                total_paths_found++;

                result.best_summary = source;
                for (const auto& hop : path.hops) {
                    result.best_summary += " -[" + hop.relation + " w=" 
                                        + std::to_string(hop.edge_weight).substr(0, 4) + "]-> " + hop.to_node;
                }
                return result;
            }

            if (d > dist[current]) continue;  // Stale entry

            auto adj_it = adjacency.find(current);
            if (adj_it == adjacency.end()) continue;

            for (int edge_idx : adj_it->second) {
                const MKTracerEdge& edge = edges[edge_idx];
                // Convert weight to distance: higher weight = shorter distance
                float edge_dist = -std::log(std::max(0.01f, edge.weight));
                float new_dist = d + edge_dist;

                if (dist.find(edge.target) == dist.end() || new_dist < dist[edge.target]) {
                    dist[edge.target] = new_dist;
                    parent[edge.target] = current;
                    parent_edge[edge.target] = edge_idx;
                    pq.push({new_dist, edge.target});
                }
            }
        }

        result.best_summary = "No weighted path found from " + source + " to " + target;
        return result;
    }

    // ─────────────────────────────────────────
    //  DFS: All paths enumeration (with limit)
    // ─────────────────────────────────────────

    MKTraceResult traceAll(const std::string& source, const std::string& target) {
        total_traces++;
        MKTraceResult result;
        result.found = false;
        result.nodes_explored = 0;
        result.paths_found = 0;

        // DFS with backtracking to find all paths
        std::vector<MKPathHop> current_path;
        std::unordered_set<std::string> visited;
        visited.insert(source);

        std::function<void(const std::string&)> dfs = [&](const std::string& current) {
            if (result.paths_found >= max_paths) return;
            if (static_cast<int>(current_path.size()) >= max_path_length) return;

            auto adj_it = adjacency.find(current);
            if (adj_it == adjacency.end()) return;

            for (int edge_idx : adj_it->second) {
                const MKTracerEdge& edge = edges[edge_idx];
                if (visited.count(edge.target)) continue;

                result.nodes_explored++;

                MKPathHop hop;
                hop.from_node = current;
                hop.to_node = edge.target;
                hop.relation = edge.relation;
                hop.edge_weight = edge.weight;
                hop.hop_number = static_cast<int>(current_path.size()) + 1;
                current_path.push_back(hop);

                if (edge.target == target) {
                    // Found a path
                    MKTracedPath traced;
                    traced.source = source;
                    traced.target = target;
                    traced.hops = current_path;
                    traced.length = static_cast<int>(current_path.size());
                    traced.total_weight = 1.0f;
                    for (const auto& h : current_path) traced.total_weight *= h.edge_weight;
                    traced.path_score = traced.total_weight / std::max(1, traced.length);
                    traced.algorithm_used = "DFS";

                    result.paths.push_back(traced);
                    result.paths_found++;
                    total_paths_found++;
                    result.found = true;
                } else {
                    visited.insert(edge.target);
                    dfs(edge.target);
                    visited.erase(edge.target);
                }

                current_path.pop_back();
            }
        };

        dfs(source);

        // Sort paths by score (highest first)
        std::sort(result.paths.begin(), result.paths.end(),
                  [](const MKTracedPath& a, const MKTracedPath& b) {
                      return a.path_score > b.path_score;
                  });

        if (!result.paths.empty()) {
            const auto& best = result.paths[0];
            result.best_summary = source;
            for (const auto& hop : best.hops) {
                result.best_summary += " -[" + hop.relation + "]-> " + hop.to_node;
            }
            result.best_summary += " (score: " + std::to_string(best.path_score).substr(0, 5) + ")";
        } else {
            result.best_summary = "No paths found from " + source + " to " + target;
        }

        return result;
    }

    // ─────────────────────────────────────────
    //  CYCLE DETECTION
    // ─────────────────────────────────────────

    std::vector<MKCycleInfo> detectCycles(int maxCycleLength = 5) {
        std::vector<MKCycleInfo> cycles;
        std::unordered_set<std::string> all_nodes;
        
        for (const auto& pair : adjacency) {
            all_nodes.insert(pair.first);
        }

        for (const auto& start : all_nodes) {
            // DFS from each node looking for back-edges
            std::unordered_set<std::string> visited;
            std::vector<std::string> path_nodes;
            std::vector<std::string> path_relations;

            std::function<void(const std::string&)> findCycles = [&](const std::string& current) {
                if (static_cast<int>(path_nodes.size()) > maxCycleLength) return;
                if (static_cast<int>(cycles.size()) > 50) return;  // Limit total cycles found

                auto adj_it = adjacency.find(current);
                if (adj_it == adjacency.end()) return;

                for (int edge_idx : adj_it->second) {
                    const MKTracerEdge& edge = edges[edge_idx];
                    
                    if (edge.target == start && path_nodes.size() >= 2) {
                        // Found a cycle back to start
                        MKCycleInfo cycle;
                        cycle.nodes = path_nodes;
                        cycle.nodes.push_back(current);
                        cycle.relations = path_relations;
                        cycle.relations.push_back(edge.relation);
                        cycle.length = static_cast<int>(cycle.nodes.size());
                        
                        float total_w = 0.0f;
                        for (const auto& rel : cycle.relations) {
                            (void)rel;
                            total_w += edge.weight;
                        }
                        cycle.avg_weight = total_w / std::max(1, cycle.length);
                        cycles.push_back(cycle);
                        return;
                    }

                    if (!visited.count(edge.target)) {
                        visited.insert(edge.target);
                        path_nodes.push_back(current);
                        path_relations.push_back(edge.relation);
                        findCycles(edge.target);
                        path_nodes.pop_back();
                        path_relations.pop_back();
                        visited.erase(edge.target);
                    }
                }
            };

            visited.insert(start);
            findCycles(start);
        }

        return cycles;
    }

    // ─────────────────────────────────────────
    //  UNIFIED TRACE (tries multiple algorithms)
    // ─────────────────────────────────────────

    MKTraceResult trace(const std::string& source, const std::string& target) {
        // First try BFS for shortest path
        MKTraceResult bfs_result = traceBFS(source, target);
        if (bfs_result.found) {
            // Also try weighted to see if there's a higher-confidence path
            MKTraceResult weighted_result = traceWeighted(source, target);
            if (weighted_result.found && !weighted_result.paths.empty() &&
                !bfs_result.paths.empty()) {
                // Return the one with better score
                if (weighted_result.paths[0].path_score > bfs_result.paths[0].path_score) {
                    return weighted_result;
                }
            }
            return bfs_result;
        }
        // If BFS failed, try weighted (might find path through different heuristic)
        return traceWeighted(source, target);
    }

    // ─────────────────────────────────────────
    //  STATS
    // ─────────────────────────────────────────

    int getEdgeCount() const { return static_cast<int>(edges.size()); }
    int getNodeCount() const { return static_cast<int>(adjacency.size()); }
    int getTotalTraces() const { return total_traces; }
    int getTotalPathsFound() const { return total_paths_found; }

    void printStats() const {
        std::cout << "\n--- [PATH TRACER] ---\n";
        std::cout << "  Edges: " << edges.size() << "\n";
        std::cout << "  Nodes: " << adjacency.size() << "\n";
        std::cout << "  Total traces: " << total_traces << "\n";
        std::cout << "  Total paths found: " << total_paths_found << "\n";
        std::cout << "  Max path length: " << max_path_length << "\n";
        std::cout << "---------------------\n";
    }
};

#endif // MK_PATH_TRACER_CPP
