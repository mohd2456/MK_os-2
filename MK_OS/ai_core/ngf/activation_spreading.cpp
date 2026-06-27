#ifndef MK_ACTIVATION_SPREADING_CPP
#define MK_ACTIVATION_SPREADING_CPP

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <cmath>
#include <algorithm>
#include <functional>

// ===================================================================================
// MK NEURAL GRAPH FUSION — ACTIVATION SPREADING
// ===================================================================================
// Spreading activation through the knowledge graph. When a concept is activated,
// energy propagates to connected nodes with configurable decay and thresholds.
//
// Algorithm:
//   1. Seed source nodes with initial activation energy
//   2. Each tick: activation spreads along edges, multiplied by edge weight * decay
//   3. Nodes below threshold are pruned (no further spreading)
//   4. Converges when no new significant activations occur
//
// This is biologically inspired (like neural spreading in the brain) but operates
// on a symbolic knowledge graph rather than a connectionist network.
// ===================================================================================

// Activation state for a single node
struct MKActivationState {
    std::string node_name;
    float energy;               // Current activation level (0.0 to unbounded, practical max ~10.0)
    float previous_energy;      // Energy at last tick (for convergence detection)
    int activated_at_tick;      // When this node first became active
    int source_distance;        // Hops from nearest source node
    bool is_source;             // Was this a seed node?
    std::string activated_by;   // Which node spread activation here
};

// Configuration for the spreading process
struct MKSpreadConfig {
    float decay_rate;           // Energy multiplier per hop (0.0 - 1.0, default 0.7)
    float threshold;            // Minimum energy to continue spreading (default 0.05)
    int max_ticks;              // Maximum iterations before forced stop (default 50)
    int max_hops;               // Maximum distance from source (default 8)
    float convergence_delta;    // Stop when total change < this (default 0.001)
    bool bidirectional;         // Spread along both incoming and outgoing edges
    float edge_weight_power;    // Exponent for edge weight influence (default 1.0)
    
    MKSpreadConfig()
        : decay_rate(0.7f), threshold(0.05f), max_ticks(50), max_hops(8),
          convergence_delta(0.001f), bidirectional(false), edge_weight_power(1.0f) {}
};

// Result of a spreading activation run
struct MKSpreadResult {
    std::unordered_map<std::string, MKActivationState> activated_nodes;
    int ticks_elapsed;
    bool converged;
    float total_energy;
    int nodes_activated;
    std::vector<std::string> activation_order;  // Order nodes were first activated
};

// Edge representation for spreading (simplified from MKEdge)
struct MKSpreadEdge {
    std::string source;
    std::string target;
    std::string relation;
    float weight;
};

class MKActivationSpreading {
private:
    // Internal graph representation for fast traversal
    std::unordered_map<std::string, std::vector<int>> outgoing_index;  // node -> edge indices
    std::unordered_map<std::string, std::vector<int>> incoming_index;  // node -> edge indices
    std::vector<MKSpreadEdge> edges;
    
    MKSpreadConfig config;
    int total_runs;
    int total_nodes_activated;

    // Compute the activation energy to spread along an edge
    float computeSpreadEnergy(float source_energy, float edge_weight, int hop_distance) {
        float weight_factor = std::pow(edge_weight, config.edge_weight_power);
        float distance_decay = std::pow(config.decay_rate, static_cast<float>(hop_distance));
        return source_energy * weight_factor * distance_decay;
    }

    // Check if the system has converged
    bool hasConverged(const std::unordered_map<std::string, MKActivationState>& states) {
        float total_delta = 0.0f;
        for (const auto& pair : states) {
            total_delta += std::abs(pair.second.energy - pair.second.previous_energy);
        }
        return total_delta < config.convergence_delta;
    }

public:
    MKActivationSpreading() : total_runs(0), total_nodes_activated(0) {
        std::cout << "[ACTIVATION SPREADING] Neural graph activation engine initialized.\n";
    }

    MKActivationSpreading(const MKSpreadConfig& cfg) 
        : config(cfg), total_runs(0), total_nodes_activated(0) {
        std::cout << "[ACTIVATION SPREADING] Engine initialized with custom config.\n";
        std::cout << "  Decay: " << config.decay_rate << ", Threshold: " << config.threshold 
                  << ", Max hops: " << config.max_hops << "\n";
    }

    // ─────────────────────────────────────────
    //  GRAPH BUILDING (from external knowledge)
    // ─────────────────────────────────────────

    // Add an edge to the internal spreading graph
    void addEdge(const std::string& source, const std::string& relation,
                 const std::string& target, float weight = 1.0f) {
        MKSpreadEdge edge;
        edge.source = source;
        edge.relation = relation;
        edge.target = target;
        edge.weight = std::max(0.0f, std::min(1.0f, weight));  // Clamp to [0,1]

        int idx = static_cast<int>(edges.size());
        edges.push_back(edge);

        outgoing_index[source].push_back(idx);
        incoming_index[target].push_back(idx);
    }

    // Bulk load edges (e.g., from MKPatternGraph)
    void loadEdges(const std::vector<MKSpreadEdge>& new_edges) {
        for (const auto& edge : new_edges) {
            addEdge(edge.source, edge.relation, edge.target, edge.weight);
        }
        std::cout << "[ACTIVATION SPREADING] Loaded " << new_edges.size() << " edges. "
                  << "Total: " << edges.size() << "\n";
    }

    // Clear all edges
    void clear() {
        edges.clear();
        outgoing_index.clear();
        incoming_index.clear();
    }

    // ─────────────────────────────────────────
    //  CORE: SPREADING ACTIVATION
    // ─────────────────────────────────────────

    // Spread activation from a single source node
    MKSpreadResult spread(const std::string& source, float initial_energy = 1.0f) {
        std::vector<std::pair<std::string, float>> sources = {{source, initial_energy}};
        return spreadMulti(sources);
    }

    // Spread activation from multiple source nodes simultaneously
    MKSpreadResult spreadMulti(const std::vector<std::pair<std::string, float>>& sources) {
        total_runs++;
        MKSpreadResult result;
        result.ticks_elapsed = 0;
        result.converged = false;
        result.total_energy = 0.0f;
        result.nodes_activated = 0;

        // Initialize source nodes
        for (const auto& src : sources) {
            MKActivationState state;
            state.node_name = src.first;
            state.energy = src.second;
            state.previous_energy = 0.0f;
            state.activated_at_tick = 0;
            state.source_distance = 0;
            state.is_source = true;
            state.activated_by = "seed";
            result.activated_nodes[src.first] = state;
            result.activation_order.push_back(src.first);
        }

        // Spreading loop
        for (int tick = 0; tick < config.max_ticks; tick++) {
            result.ticks_elapsed = tick + 1;

            // Save previous energies for convergence check
            for (auto& pair : result.activated_nodes) {
                pair.second.previous_energy = pair.second.energy;
            }

            // Collect new activations for this tick
            std::unordered_map<std::string, float> new_activations;
            std::unordered_map<std::string, std::string> new_activated_by;

            // For each currently active node, spread to neighbors
            for (const auto& pair : result.activated_nodes) {
                const std::string& node = pair.first;
                float energy = pair.second.energy;
                int distance = pair.second.source_distance;

                if (energy < config.threshold) continue;
                if (distance >= config.max_hops) continue;

                // Spread along outgoing edges
                auto out_it = outgoing_index.find(node);
                if (out_it != outgoing_index.end()) {
                    for (int edge_idx : out_it->second) {
                        const MKSpreadEdge& edge = edges[edge_idx];
                        float spread_energy = computeSpreadEnergy(energy, edge.weight, distance + 1);
                        
                        if (spread_energy >= config.threshold) {
                            if (new_activations.find(edge.target) == new_activations.end() ||
                                new_activations[edge.target] < spread_energy) {
                                new_activations[edge.target] = spread_energy;
                                new_activated_by[edge.target] = node;
                            }
                        }
                    }
                }

                // Optionally spread along incoming edges (bidirectional)
                if (config.bidirectional) {
                    auto in_it = incoming_index.find(node);
                    if (in_it != incoming_index.end()) {
                        for (int edge_idx : in_it->second) {
                            const MKSpreadEdge& edge = edges[edge_idx];
                            float spread_energy = computeSpreadEnergy(energy, edge.weight * 0.5f, distance + 1);
                            
                            if (spread_energy >= config.threshold) {
                                if (new_activations.find(edge.source) == new_activations.end() ||
                                    new_activations[edge.source] < spread_energy) {
                                    new_activations[edge.source] = spread_energy;
                                    new_activated_by[edge.source] = node;
                                }
                            }
                        }
                    }
                }
            }

            // Apply new activations (accumulate energy for already-active nodes)
            bool any_new = false;
            for (const auto& act : new_activations) {
                auto it = result.activated_nodes.find(act.first);
                if (it != result.activated_nodes.end()) {
                    // Already active: add energy (capped to prevent explosion)
                    it->second.energy = std::min(10.0f, it->second.energy + act.second * 0.5f);
                } else {
                    // New activation
                    MKActivationState state;
                    state.node_name = act.first;
                    state.energy = act.second;
                    state.previous_energy = 0.0f;
                    state.activated_at_tick = tick + 1;
                    state.is_source = false;
                    state.activated_by = new_activated_by[act.first];
                    
                    // Calculate distance from source
                    auto parent_it = result.activated_nodes.find(state.activated_by);
                    state.source_distance = (parent_it != result.activated_nodes.end()) 
                        ? parent_it->second.source_distance + 1 : 1;
                    
                    result.activated_nodes[act.first] = state;
                    result.activation_order.push_back(act.first);
                    any_new = true;
                }
            }

            // Apply natural energy decay to all nodes each tick
            for (auto& pair : result.activated_nodes) {
                if (!pair.second.is_source) {
                    pair.second.energy *= 0.95f;  // 5% natural decay per tick
                }
            }

            // Check convergence
            if (!any_new && hasConverged(result.activated_nodes)) {
                result.converged = true;
                break;
            }
        }

        // Calculate final statistics
        result.total_energy = 0.0f;
        for (const auto& pair : result.activated_nodes) {
            if (pair.second.energy >= config.threshold) {
                result.total_energy += pair.second.energy;
                result.nodes_activated++;
            }
        }
        total_nodes_activated += result.nodes_activated;

        return result;
    }

    // ─────────────────────────────────────────
    //  UTILITIES
    // ─────────────────────────────────────────

    // Get the top N most activated nodes from a result
    std::vector<MKActivationState> getTopActivated(const MKSpreadResult& result, int topN = 10) {
        std::vector<MKActivationState> sorted;
        for (const auto& pair : result.activated_nodes) {
            if (pair.second.energy >= config.threshold) {
                sorted.push_back(pair.second);
            }
        }
        std::sort(sorted.begin(), sorted.end(), 
                  [](const MKActivationState& a, const MKActivationState& b) {
                      return a.energy > b.energy;
                  });
        if (static_cast<int>(sorted.size()) > topN) {
            sorted.resize(topN);
        }
        return sorted;
    }

    // Find nodes activated by a specific relation type
    std::vector<MKActivationState> getActivatedByRelation(const MKSpreadResult& result,
                                                           const std::string& relation) {
        std::vector<MKActivationState> filtered;
        for (const auto& pair : result.activated_nodes) {
            if (pair.second.energy < config.threshold) continue;
            // Check if this node was reached via the specified relation
            auto out_it = incoming_index.find(pair.first);
            if (out_it != incoming_index.end()) {
                for (int idx : out_it->second) {
                    if (edges[idx].relation == relation) {
                        filtered.push_back(pair.second);
                        break;
                    }
                }
            }
        }
        return filtered;
    }

    // Get the activation path from source to a specific target
    std::vector<std::string> getActivationPath(const MKSpreadResult& result,
                                                const std::string& target) {
        std::vector<std::string> path;
        std::string current = target;
        
        std::unordered_set<std::string> visited;
        while (!current.empty() && current != "seed") {
            if (visited.count(current)) break;  // Prevent infinite loops
            visited.insert(current);
            path.push_back(current);
            auto it = result.activated_nodes.find(current);
            if (it != result.activated_nodes.end()) {
                current = it->second.activated_by;
            } else {
                break;
            }
        }
        std::reverse(path.begin(), path.end());
        return path;
    }

    // Update spreading configuration
    void setConfig(const MKSpreadConfig& cfg) { config = cfg; }
    MKSpreadConfig getConfig() const { return config; }

    // Stats
    int getEdgeCount() const { return static_cast<int>(edges.size()); }
    int getNodeCount() const { return static_cast<int>(outgoing_index.size()); }
    int getTotalRuns() const { return total_runs; }
    int getTotalNodesActivated() const { return total_nodes_activated; }

    void printStats() const {
        std::cout << "\n--- [ACTIVATION SPREADING] ---\n";
        std::cout << "  Edges: " << edges.size() << "\n";
        std::cout << "  Nodes: " << outgoing_index.size() << "\n";
        std::cout << "  Total runs: " << total_runs << "\n";
        std::cout << "  Total nodes activated: " << total_nodes_activated << "\n";
        std::cout << "  Config: decay=" << config.decay_rate << " threshold=" << config.threshold
                  << " max_hops=" << config.max_hops << "\n";
        std::cout << "------------------------------\n";
    }
};

#endif // MK_ACTIVATION_SPREADING_CPP
