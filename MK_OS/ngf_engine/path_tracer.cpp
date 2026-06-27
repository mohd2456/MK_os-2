#ifndef MK_PATH_TRACER_CPP
#define MK_PATH_TRACER_CPP

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <chrono>
#include <functional>
#include <numeric>
#include <stack>

// ===================================================================================
// MK NEURAL GRAPH FUSION - PATH TRACER
// ===================================================================================
// After activation spreading identifies high-activation zones, this module traces
// the strongest paths between input concepts and those zones. The traced path IS
// the reasoning chain - it shows HOW the system connected input to answer.
//
// Features:
//   - Traces strongest energy gradient paths
//   - Returns ordered list of concepts + relationships
//   - Multi-path tracing for complex reasoning
//   - Path scoring and confidence estimation
//   - Cycle detection and avoidance
//   - Branch-and-bound for efficiency
// ===================================================================================

// A single step in a reasoning path
struct MKPathStep {
    std::string concept;           // The concept at this step
    std::string relation_to_next;  // Relation connecting to next step
    float energy_at_step;          // Activation energy at this node
    float edge_weight;             // Weight of edge to next node
    int depth;                     // Depth in the reasoning chain

    MKPathStep() : energy_at_step(0.0f), edge_weight(0.0f), depth(0) {}
    MKPathStep(const std::string& c, const std::string& r, float e, float w, int d)
        : concept(c), relation_to_next(r), energy_at_step(e), edge_weight(w), depth(d) {}
};

// A complete reasoning path from source to target
struct MKReasoningPath {
    std::vector<MKPathStep> steps;
    std::string source;
    std::string target;
    float total_energy;            // Sum of energy along path
    float path_confidence;         // Overall confidence score
    float path_coherence;          // How well steps relate to each other
    int hop_count;                 // Number of hops
    bool is_direct;                // True if single-hop
    std::string summary;           // Brief text summary of the path

    MKReasoningPath()
        : total_energy(0.0f), path_confidence(0.0f),
          path_coherence(0.0f), hop_count(0), is_direct(false) {}

    // Calculate overall score combining energy, confidence, and brevity
    float getScore() const {
        float brevity_bonus = 1.0f / (1.0f + hop_count * 0.1f);
        return total_energy * path_confidence * brevity_bonus;
    }
};

// Result of path tracing
struct MKTraceResult {
    bool success;
    std::vector<MKReasoningPath> paths;       // All found paths, ranked by score
    MKReasoningPath best_path;                // The single best path
    int total_paths_explored;
    int total_nodes_visited;
    double trace_time_ms;
    float overall_confidence;

    MKTraceResult()
        : success(false), total_paths_explored(0),
          total_nodes_visited(0), trace_time_ms(0.0),
          overall_confidence(0.0f) {}
};

// Configuration for path tracing
struct MKTraceConfig {
    int max_path_length;           // Maximum hops in a single path
    int max_paths_to_find;         // Maximum number of paths to return
    float min_energy_threshold;    // Minimum energy to follow a path
    float min_path_confidence;     // Minimum confidence to keep a path
    bool allow_cycles;             // Whether to allow revisiting nodes
    bool prefer_short_paths;       // Bias toward shorter paths
    float energy_gradient_weight;  // How much to follow energy gradient vs. BFS

    MKTraceConfig()
        : max_path_length(8), max_paths_to_find(5),
          min_energy_threshold(0.05f), min_path_confidence(0.1f),
          allow_cycles(false), prefer_short_paths(true),
          energy_gradient_weight(0.7f) {}
};

// The main path tracer class
class MKPathTracer {
private:
    MKTraceConfig config_;
    size_t total_traces_;
    size_t total_paths_found_;

    // Relation type names for path descriptions
    std::unordered_map<std::string, std::string> relation_labels_;

    // Score a partial path for priority queue ordering
    float scorePath(const std::vector<MKPathStep>& steps, float target_energy) const {
        if (steps.empty()) return 0.0f;

        float energy_score = 0.0f;
        for (const auto& step : steps) {
            energy_score += step.energy_at_step;
        }
        energy_score /= steps.size();

        float length_penalty = config_.prefer_short_paths ?
            (1.0f / (1.0f + steps.size() * 0.15f)) : 1.0f;

        float gradient_score = steps.back().energy_at_step / (steps[0].energy_at_step + 0.001f);
        if (gradient_score > 1.0f) gradient_score = 1.0f;

        return energy_score * length_penalty * (1.0f + gradient_score * 0.5f) + target_energy * 0.3f;
    }

    // Build a summary string for a path
    std::string buildPathSummary(const MKReasoningPath& path) const {
        if (path.steps.empty()) return "";
        std::ostringstream oss;
        for (size_t i = 0; i < path.steps.size(); i++) {
            oss << path.steps[i].concept;
            if (i < path.steps.size() - 1 && !path.steps[i].relation_to_next.empty()) {
                oss << " -[" << path.steps[i].relation_to_next << "]-> ";
            }
        }
        return oss.str();
    }

    // Calculate path coherence (how related successive concepts are)
    float calculateCoherence(const std::vector<MKPathStep>& steps) const {
        if (steps.size() < 2) return 1.0f;
        float coherence = 0.0f;
        int transitions = 0;
        for (size_t i = 0; i < steps.size() - 1; i++) {
            // Higher edge weight = more coherent transition
            coherence += steps[i].edge_weight;
            transitions++;
        }
        return transitions > 0 ? coherence / transitions : 0.0f;
    }

public:
    MKPathTracer() : total_traces_(0), total_paths_found_(0) {
        std::cout << "[NGF PathTracer] Initialized path tracer" << std::endl;
        initRelationLabels();
    }

    MKPathTracer(const MKTraceConfig& config) : config_(config), total_traces_(0), total_paths_found_(0) {
        std::cout << "[NGF PathTracer] Initialized with custom config" << std::endl;
        initRelationLabels();
    }

    void initRelationLabels() {
        relation_labels_ = {
            {"is_a", "is a type of"},
            {"causes", "causes"},
            {"enables", "enables"},
            {"prevents", "prevents"},
            {"part_of", "is part of"},
            {"has_property", "has the property of"},
            {"opposite_of", "is the opposite of"},
            {"used_for", "is used for"},
            {"made_of", "is made of"},
            {"located_in", "is located in"},
            {"requires", "requires"},
            {"produces", "produces"},
            {"similar_to", "is similar to"},
            {"contains", "contains"},
            {"precedes", "comes before"},
            {"follows", "comes after"},
            {"implies", "implies"},
            {"contradicts", "contradicts"},
            {"supports", "supports"},
            {"derived_from", "is derived from"},
            {"example_of", "is an example of"},
            {"synonym_of", "is a synonym of"}
        };
    }

    void setConfig(const MKTraceConfig& config) { config_ = config; }

    // Trace strongest path between source and target using activation energy map
    MKTraceResult tracePath(
        const std::string& source,
        const std::string& target,
        const std::unordered_map<std::string, float>& energy_map,
        const std::unordered_map<std::string, std::vector<std::pair<std::string, float>>>& graph,
        const std::unordered_map<std::string, std::string>& edge_relations = {}) {

        auto start_time = std::chrono::high_resolution_clock::now();
        MKTraceResult result;
        total_traces_++;

        if (graph.find(source) == graph.end()) {
            return result;
        }

        // Priority queue: (negative_score, path_so_far)
        struct TraceEntry {
            float score;
            std::vector<MKPathStep> path;
            std::unordered_set<std::string> visited;

            bool operator<(const TraceEntry& other) const {
                return score < other.score;  // max-heap
            }
        };

        std::priority_queue<TraceEntry> pq;

        // Initialize with source
        TraceEntry initial;
        float source_energy = 0.0f;
        auto eit = energy_map.find(source);
        if (eit != energy_map.end()) source_energy = eit->second;

        MKPathStep first_step(source, "", source_energy, 0.0f, 0);
        initial.path.push_back(first_step);
        initial.visited.insert(source);
        initial.score = source_energy;
        pq.push(initial);

        int nodes_visited = 0;
        int paths_explored = 0;

        while (!pq.empty() && (int)result.paths.size() < config_.max_paths_to_find) {
            TraceEntry current = pq.top();
            pq.pop();
            paths_explored++;

            if (paths_explored > 10000) break;  // Safety limit

            std::string current_node = current.path.back().concept;

            // Check if we reached the target
            if (current_node == target && current.path.size() > 1) {
                MKReasoningPath rpath;
                rpath.steps = current.path;
                rpath.source = source;
                rpath.target = target;
                rpath.hop_count = (int)current.path.size() - 1;
                rpath.is_direct = (rpath.hop_count == 1);

                // Calculate path metrics
                float total_e = 0.0f;
                for (const auto& step : rpath.steps) total_e += step.energy_at_step;
                rpath.total_energy = total_e;
                rpath.path_coherence = calculateCoherence(current.path);
                rpath.path_confidence = rpath.path_coherence * (total_e / current.path.size());
                rpath.summary = buildPathSummary(rpath);

                result.paths.push_back(rpath);
                total_paths_found_++;
                continue;
            }

            // Check path length limit
            if ((int)current.path.size() >= config_.max_path_length) continue;

            // Expand neighbors
            auto git = graph.find(current_node);
            if (git == graph.end()) continue;

            for (const auto& [neighbor, edge_weight] : git->second) {
                if (!config_.allow_cycles && current.visited.count(neighbor)) continue;

                float neighbor_energy = 0.0f;
                auto neit = energy_map.find(neighbor);
                if (neit != energy_map.end()) neighbor_energy = neit->second;

                if (neighbor_energy < config_.min_energy_threshold && neighbor != target) continue;

                // Get relation label
                std::string relation = "";
                std::string edge_key = current_node + "|" + neighbor;
                auto rit = edge_relations.find(edge_key);
                if (rit != edge_relations.end()) relation = rit->second;

                // Update the last step's relation_to_next
                TraceEntry next;
                next.path = current.path;
                next.path.back().relation_to_next = relation;
                next.path.back().edge_weight = edge_weight;

                MKPathStep step(neighbor, "", neighbor_energy, 0.0f, (int)next.path.size());
                next.path.push_back(step);
                next.visited = current.visited;
                next.visited.insert(neighbor);

                float target_e = 0.0f;
                auto teit = energy_map.find(target);
                if (teit != energy_map.end()) target_e = teit->second;
                next.score = scorePath(next.path, target_e);

                nodes_visited++;
                pq.push(next);
            }
        }

        // Sort paths by score
        std::sort(result.paths.begin(), result.paths.end(),
                 [](const MKReasoningPath& a, const MKReasoningPath& b) {
                     return a.getScore() > b.getScore();
                 });

        result.success = !result.paths.empty();
        result.total_paths_explored = paths_explored;
        result.total_nodes_visited = nodes_visited;

        if (result.success) {
            result.best_path = result.paths[0];
            float conf_sum = 0.0f;
            for (const auto& p : result.paths) conf_sum += p.path_confidence;
            result.overall_confidence = conf_sum / result.paths.size();
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        result.trace_time_ms = std::chrono::duration<double, std::milli>(
            end_time - start_time).count();

        return result;
    }

    // Trace paths from multiple sources to multiple targets
    MKTraceResult traceMultiple(
        const std::vector<std::string>& sources,
        const std::vector<std::string>& targets,
        const std::unordered_map<std::string, float>& energy_map,
        const std::unordered_map<std::string, std::vector<std::pair<std::string, float>>>& graph,
        const std::unordered_map<std::string, std::string>& edge_relations = {}) {

        MKTraceResult combined;
        auto start_time = std::chrono::high_resolution_clock::now();

        for (const auto& src : sources) {
            for (const auto& tgt : targets) {
                if (src == tgt) continue;
                MKTraceResult partial = tracePath(src, tgt, energy_map, graph, edge_relations);
                if (partial.success) {
                    for (auto& path : partial.paths) {
                        combined.paths.push_back(path);
                    }
                }
                combined.total_paths_explored += partial.total_paths_explored;
                combined.total_nodes_visited += partial.total_nodes_visited;
            }
        }

        // Sort all paths by score
        std::sort(combined.paths.begin(), combined.paths.end(),
                 [](const MKReasoningPath& a, const MKReasoningPath& b) {
                     return a.getScore() > b.getScore();
                 });

        // Limit to max_paths_to_find
        if ((int)combined.paths.size() > config_.max_paths_to_find) {
            combined.paths.resize(config_.max_paths_to_find);
        }

        combined.success = !combined.paths.empty();
        if (combined.success) {
            combined.best_path = combined.paths[0];
            float conf_sum = 0.0f;
            for (const auto& p : combined.paths) conf_sum += p.path_confidence;
            combined.overall_confidence = conf_sum / combined.paths.size();
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        combined.trace_time_ms = std::chrono::duration<double, std::milli>(
            end_time - start_time).count();

        return combined;
    }

    // Trace the strongest gradient path (always follow highest energy neighbor)
    MKReasoningPath traceGradient(
        const std::string& source,
        const std::unordered_map<std::string, float>& energy_map,
        const std::unordered_map<std::string, std::vector<std::pair<std::string, float>>>& graph,
        const std::unordered_map<std::string, std::string>& edge_relations = {}) {

        MKReasoningPath path;
        path.source = source;
        std::unordered_set<std::string> visited;
        std::string current = source;

        float source_energy = 0.0f;
        auto eit = energy_map.find(source);
        if (eit != energy_map.end()) source_energy = eit->second;

        path.steps.emplace_back(current, "", source_energy, 0.0f, 0);
        visited.insert(current);

        for (int hop = 0; hop < config_.max_path_length; hop++) {
            auto git = graph.find(current);
            if (git == graph.end()) break;

            // Find highest-energy unvisited neighbor
            std::string best_next = "";
            float best_energy = -1.0f;
            float best_edge_weight = 0.0f;

            for (const auto& [neighbor, ew] : git->second) {
                if (visited.count(neighbor)) continue;
                float ne = 0.0f;
                auto neit = energy_map.find(neighbor);
                if (neit != energy_map.end()) ne = neit->second;
                float combined = ne * ew;
                if (combined > best_energy) {
                    best_energy = combined;
                    best_next = neighbor;
                    best_edge_weight = ew;
                }
            }

            if (best_next.empty() || best_energy < config_.min_energy_threshold) break;

            // Get relation
            std::string relation = "";
            std::string edge_key = current + "|" + best_next;
            auto rit = edge_relations.find(edge_key);
            if (rit != edge_relations.end()) relation = rit->second;

            path.steps.back().relation_to_next = relation;
            path.steps.back().edge_weight = best_edge_weight;

            float next_energy = 0.0f;
            auto neit = energy_map.find(best_next);
            if (neit != energy_map.end()) next_energy = neit->second;

            path.steps.emplace_back(best_next, "", next_energy, 0.0f, hop + 1);
            visited.insert(best_next);
            current = best_next;
        }

        path.target = current;
        path.hop_count = (int)path.steps.size() - 1;
        path.is_direct = (path.hop_count == 1);

        float total_e = 0.0f;
        for (const auto& step : path.steps) total_e += step.energy_at_step;
        path.total_energy = total_e;
        path.path_coherence = calculateCoherence(path.steps);
        path.path_confidence = path.path_coherence * (total_e / std::max((int)path.steps.size(), 1));
        path.summary = buildPathSummary(path);

        return path;
    }

    // Get a human-readable description of a path
    std::string describePath(const MKReasoningPath& path) const {
        if (path.steps.empty()) return "No path found.";
        std::ostringstream oss;
        oss << "Reasoning chain (" << path.hop_count << " steps, confidence: "
            << (int)(path.path_confidence * 100) << "%):\n";
        for (size_t i = 0; i < path.steps.size(); i++) {
            oss << "  " << (i + 1) << ". " << path.steps[i].concept;
            if (i < path.steps.size() - 1 && !path.steps[i].relation_to_next.empty()) {
                auto lit = relation_labels_.find(path.steps[i].relation_to_next);
                if (lit != relation_labels_.end()) {
                    oss << " (" << lit->second << ")";
                } else {
                    oss << " [" << path.steps[i].relation_to_next << "]";
                }
            }
            oss << "\n";
        }
        return oss.str();
    }

    // Get statistics
    size_t getTotalTraces() const { return total_traces_; }
    size_t getTotalPathsFound() const { return total_paths_found_; }

    // Print trace summary
    void printSummary(const MKTraceResult& result) const {
        std::cout << "[NGF PathTracer] === Trace Summary ===" << std::endl;
        std::cout << "  Success: " << (result.success ? "yes" : "no") << std::endl;
        std::cout << "  Paths found: " << result.paths.size() << std::endl;
        std::cout << "  Nodes visited: " << result.total_nodes_visited << std::endl;
        std::cout << "  Time: " << result.trace_time_ms << " ms" << std::endl;
        if (result.success) {
            std::cout << "  Best path score: " << result.best_path.getScore() << std::endl;
            std::cout << "  Best path hops: " << result.best_path.hop_count << std::endl;
            std::cout << "  Overall confidence: " << result.overall_confidence << std::endl;
        }
    }
};

#endif // MK_PATH_TRACER_CPP
