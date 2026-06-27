#ifndef MK_ACTIVATION_ENGINE_CPP
#define MK_ACTIVATION_ENGINE_CPP

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

// ===================================================================================
// MK NEURAL GRAPH FUSION - ACTIVATION ENGINE
// ===================================================================================
// When input comes in, this engine splits it into concept atoms, then spreads
// activation energy through the concept graph. Energy weakens over distance (decay).
// Where energy from MULTIPLE input words converges = high activation = the answer.
//
// Algorithm:
//   1. Parse input into concept tokens (atoms)
//   2. Seed initial activation at each concept's node
//   3. BFS spreading with configurable decay factor per hop
//   4. Track convergence zones where multiple sources meet
//   5. Return top-activated nodes ranked by combined energy
//
// Key parameters:
//   - decay_factor: energy multiplied by this each hop (default 0.6)
//   - activation_threshold: minimum energy to continue spreading (default 0.01)
//   - max_spread_depth: maximum hops from source (default 5)
//   - convergence_bonus: multiplier when multiple sources activate same node (2.0)
// ===================================================================================

// Activation state for a single node
struct MKActivationState {
    float energy;                       // Current activation energy
    int sources_count;                  // How many different sources activated this node
    std::vector<std::string> sources;   // Which source concepts activated this
    int depth_from_source;              // Minimum depth from any source
    float max_single_source;            // Highest single-source contribution
    bool is_convergence_point;          // True if activated by multiple sources

    MKActivationState()
        : energy(0.0f), sources_count(0), depth_from_source(999),
          max_single_source(0.0f), is_convergence_point(false) {}

    void addActivation(float e, const std::string& source, int depth) {
        energy += e;
        if (std::find(sources.begin(), sources.end(), source) == sources.end()) {
            sources.push_back(source);
            sources_count++;
        }
        if (depth < depth_from_source) depth_from_source = depth;
        if (e > max_single_source) max_single_source = e;
        is_convergence_point = (sources_count > 1);
    }
};

// Result of the activation spreading process
struct MKActivationResult {
    bool success;
    std::vector<std::pair<std::string, float>> top_activated;  // node, energy
    std::vector<std::string> convergence_points;               // multi-source nodes
    int total_nodes_activated;
    int total_hops_spread;
    float max_energy;
    float avg_energy;
    double spread_time_ms;
    std::unordered_map<std::string, MKActivationState> full_state;

    MKActivationResult()
        : success(false), total_nodes_activated(0), total_hops_spread(0),
          max_energy(0.0f), avg_energy(0.0f), spread_time_ms(0.0) {}
};

// Configuration for activation spreading
struct MKActivationConfig {
    float decay_factor;          // Energy decay per hop (0.0-1.0)
    float activation_threshold;  // Minimum energy to keep spreading
    int max_spread_depth;        // Maximum hops from source
    float convergence_bonus;     // Multiplier for convergence points
    float initial_energy;        // Starting energy at source nodes
    bool bidirectional;          // Also spread through reverse edges
    int max_nodes_to_activate;   // Cap on total activated nodes
    float relation_weight_factor; // How much edge weight affects spread

    MKActivationConfig()
        : decay_factor(0.6f), activation_threshold(0.01f),
          max_spread_depth(5), convergence_bonus(2.0f),
          initial_energy(1.0f), bidirectional(false),
          max_nodes_to_activate(10000), relation_weight_factor(1.0f) {}
};

// The main activation engine class
class MKActivationEngine {
private:
    MKActivationConfig config_;
    std::unordered_map<std::string, MKActivationState> activation_map_;
    size_t total_spreads_;
    size_t total_convergences_;

    // Tokenize input into concept atoms
    std::vector<std::string> shatterInput(const std::string& input) const {
        std::vector<std::string> atoms;
        std::istringstream iss(input);
        std::string word;

        // Stop words to filter out
        static const std::unordered_set<std::string> stop_words = {
            "the", "a", "an", "is", "are", "was", "were", "be", "been",
            "being", "have", "has", "had", "do", "does", "did", "will",
            "would", "could", "should", "may", "might", "shall", "can",
            "to", "of", "in", "for", "on", "with", "at", "by", "from",
            "as", "into", "through", "during", "before", "after", "above",
            "below", "between", "out", "off", "over", "under", "again",
            "further", "then", "once", "here", "there", "when", "where",
            "why", "how", "all", "each", "every", "both", "few", "more",
            "most", "other", "some", "such", "no", "nor", "not", "only",
            "own", "same", "so", "than", "too", "very", "just", "because",
            "but", "and", "or", "if", "while", "about", "it", "its",
            "this", "that", "these", "those", "what", "which", "who"
        };

        while (iss >> word) {
            // Normalize: lowercase, remove punctuation
            std::string clean;
            for (char c : word) {
                if (std::isalnum(c)) clean += std::tolower(c);
            }
            if (clean.empty()) continue;
            if (stop_words.find(clean) != stop_words.end()) continue;
            atoms.push_back(clean);
        }

        // Also try bigrams for compound concepts
        if (atoms.size() >= 2) {
            std::vector<std::string> bigrams;
            for (size_t i = 0; i < atoms.size() - 1; i++) {
                bigrams.push_back(atoms[i] + "_" + atoms[i + 1]);
            }
            atoms.insert(atoms.end(), bigrams.begin(), bigrams.end());
        }

        return atoms;
    }

    // Calculate energy after decay at a given depth
    float calculateEnergy(float source_energy, int depth, float edge_weight) const {
        float decayed = source_energy * std::pow(config_.decay_factor, depth);
        float weighted = decayed * (edge_weight * config_.relation_weight_factor +
                                    (1.0f - config_.relation_weight_factor));
        return weighted;
    }

    // Check if a concept exists in the graph (interface method)
    // In practice, this checks the activation map or a provided graph
    bool conceptExists(const std::string& concept,
                      const std::unordered_map<std::string, std::vector<std::pair<std::string, float>>>& graph) const {
        return graph.find(concept) != graph.end();
    }

public:
    MKActivationEngine() : total_spreads_(0), total_convergences_(0) {
        std::cout << "[NGF ActivationEngine] Initialized with default config" << std::endl;
    }

    MKActivationEngine(const MKActivationConfig& config)
        : config_(config), total_spreads_(0), total_convergences_(0) {
        std::cout << "[NGF ActivationEngine] Initialized with custom config" << std::endl;
        std::cout << "  Decay: " << config_.decay_factor
                  << " Threshold: " << config_.activation_threshold
                  << " MaxDepth: " << config_.max_spread_depth << std::endl;
    }

    // Set configuration
    void setConfig(const MKActivationConfig& config) { config_ = config; }
    MKActivationConfig getConfig() const { return config_; }

    // Shatter a query into concept atoms (public interface)
    std::vector<std::string> shatter(const std::string& input) const {
        return shatterInput(input);
    }

    // Spread activation from multiple source concepts through a graph
    // The graph is passed as adjacency list: node -> [(neighbor, weight), ...]
    MKActivationResult spreadActivation(
        const std::vector<std::string>& sources,
        const std::unordered_map<std::string, std::vector<std::pair<std::string, float>>>& graph) {

        auto start_time = std::chrono::high_resolution_clock::now();
        MKActivationResult result;
        activation_map_.clear();
        total_spreads_++;

        if (sources.empty()) {
            result.success = false;
            return result;
        }

        // BFS queue entries: (node, source_concept, current_depth, current_energy)
        struct SpreadEntry {
            std::string node;
            std::string source;
            int depth;
            float energy;
        };

        std::queue<SpreadEntry> frontier;

        // Seed initial activation at source nodes
        for (const auto& src : sources) {
            if (graph.find(src) != graph.end()) {
                SpreadEntry entry;
                entry.node = src;
                entry.source = src;
                entry.depth = 0;
                entry.energy = config_.initial_energy;
                frontier.push(entry);
                activation_map_[src].addActivation(config_.initial_energy, src, 0);
            }
        }

        // Spread activation through the graph
        int nodes_activated = 0;
        int total_hops = 0;

        while (!frontier.empty() && nodes_activated < config_.max_nodes_to_activate) {
            SpreadEntry current = frontier.front();
            frontier.pop();

            if (current.depth >= config_.max_spread_depth) continue;

            auto it = graph.find(current.node);
            if (it == graph.end()) continue;

            for (const auto& [neighbor, edge_weight] : it->second) {
                float new_energy = calculateEnergy(current.energy, 1, edge_weight);

                if (new_energy < config_.activation_threshold) continue;

                bool was_inactive = (activation_map_.find(neighbor) == activation_map_.end() ||
                                    activation_map_[neighbor].energy == 0.0f);

                activation_map_[neighbor].addActivation(new_energy, current.source,
                                                        current.depth + 1);

                // Apply convergence bonus
                if (activation_map_[neighbor].sources_count > 1) {
                    activation_map_[neighbor].energy *= config_.convergence_bonus;
                    activation_map_[neighbor].is_convergence_point = true;
                }

                if (was_inactive) nodes_activated++;
                total_hops++;

                SpreadEntry next;
                next.node = neighbor;
                next.source = current.source;
                next.depth = current.depth + 1;
                next.energy = new_energy;
                frontier.push(next);
            }
        }

        // Collect results
        result.success = true;
        result.total_nodes_activated = nodes_activated;
        result.total_hops_spread = total_hops;
        result.full_state = activation_map_;

        // Sort by activation energy to find top activated nodes
        std::vector<std::pair<std::string, float>> all_activated;
        for (const auto& [node, state] : activation_map_) {
            all_activated.push_back({node, state.energy});
            if (state.is_convergence_point) {
                result.convergence_points.push_back(node);
                total_convergences_++;
            }
        }

        std::sort(all_activated.begin(), all_activated.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });

        // Top 20 activated nodes
        size_t top_count = std::min(all_activated.size(), (size_t)20);
        result.top_activated.assign(all_activated.begin(), all_activated.begin() + top_count);

        if (!all_activated.empty()) {
            result.max_energy = all_activated[0].second;
            float sum = 0.0f;
            for (const auto& [n, e] : all_activated) sum += e;
            result.avg_energy = sum / all_activated.size();
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        result.spread_time_ms = std::chrono::duration<double, std::milli>(
            end_time - start_time).count();

        return result;
    }

    // Convenience method: spread from a raw query string using a graph
    MKActivationResult spreadFromQuery(
        const std::string& query,
        const std::unordered_map<std::string, std::vector<std::pair<std::string, float>>>& graph) {

        std::vector<std::string> atoms = shatterInput(query);
        return spreadActivation(atoms, graph);
    }

    // Get activation energy for a specific node
    float getActivation(const std::string& node) const {
        auto it = activation_map_.find(node);
        if (it != activation_map_.end()) return it->second.energy;
        return 0.0f;
    }

    // Get all convergence points (nodes activated by multiple sources)
    std::vector<std::pair<std::string, float>> getConvergencePoints() const {
        std::vector<std::pair<std::string, float>> points;
        for (const auto& [node, state] : activation_map_) {
            if (state.is_convergence_point) {
                points.push_back({node, state.energy});
            }
        }
        std::sort(points.begin(), points.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        return points;
    }

    // Get top N activated nodes
    std::vector<std::pair<std::string, float>> getTopActivated(int n = 10) const {
        std::vector<std::pair<std::string, float>> all;
        for (const auto& [node, state] : activation_map_) {
            all.push_back({node, state.energy});
        }
        std::sort(all.begin(), all.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        if ((int)all.size() > n) all.resize(n);
        return all;
    }

    // Get full activation state for a node
    MKActivationState getState(const std::string& node) const {
        auto it = activation_map_.find(node);
        if (it != activation_map_.end()) return it->second;
        return MKActivationState();
    }

    // Check if activation has converged (energy changes below threshold)
    bool hasConverged(const MKActivationResult& result) const {
        // Consider converged if we found convergence points
        // or if max energy exceeds a strong threshold
        return (!result.convergence_points.empty() || result.max_energy > 0.5f);
    }

    // Reset activation state for next query
    void reset() {
        activation_map_.clear();
    }

    // Get statistics
    size_t getTotalSpreads() const { return total_spreads_; }
    size_t getTotalConvergences() const { return total_convergences_; }

    // Print activation summary
    void printSummary(const MKActivationResult& result) const {
        std::cout << "[NGF ActivationEngine] === Spread Summary ===" << std::endl;
        std::cout << "  Nodes activated: " << result.total_nodes_activated << std::endl;
        std::cout << "  Total hops: " << result.total_hops_spread << std::endl;
        std::cout << "  Max energy: " << result.max_energy << std::endl;
        std::cout << "  Avg energy: " << result.avg_energy << std::endl;
        std::cout << "  Convergence points: " << result.convergence_points.size() << std::endl;
        std::cout << "  Time: " << result.spread_time_ms << " ms" << std::endl;
        if (!result.top_activated.empty()) {
            std::cout << "  Top 5 nodes:" << std::endl;
            int shown = 0;
            for (const auto& [node, energy] : result.top_activated) {
                if (shown++ >= 5) break;
                std::cout << "    " << node << " = " << energy << std::endl;
            }
        }
    }

    // Focused activation: only spread through specific relation types
    MKActivationResult spreadFocused(
        const std::vector<std::string>& sources,
        const std::unordered_map<std::string, std::vector<std::pair<std::string, float>>>& graph,
        float boost_factor = 1.5f) {

        // Use tighter config for focused search
        MKActivationConfig focused_config = config_;
        focused_config.decay_factor = config_.decay_factor * 0.8f;
        focused_config.max_spread_depth = config_.max_spread_depth - 1;
        focused_config.initial_energy = config_.initial_energy * boost_factor;

        MKActivationConfig old_config = config_;
        config_ = focused_config;
        MKActivationResult result = spreadActivation(sources, graph);
        config_ = old_config;
        return result;
    }

    // Multi-pass activation: spread, prune weak nodes, spread again from strong nodes
    MKActivationResult spreadMultiPass(
        const std::vector<std::string>& sources,
        const std::unordered_map<std::string, std::vector<std::pair<std::string, float>>>& graph,
        int passes = 2) {

        MKActivationResult result = spreadActivation(sources, graph);

        for (int pass = 1; pass < passes; pass++) {
            // Get strong nodes from previous pass
            std::vector<std::string> strong_nodes;
            for (const auto& [node, energy] : result.top_activated) {
                if (energy > result.avg_energy * 2.0f) {
                    strong_nodes.push_back(node);
                }
            }
            if (strong_nodes.empty()) break;

            // Spread from strong nodes with reduced energy
            MKActivationConfig pass_config = config_;
            pass_config.initial_energy = config_.initial_energy * 0.5f;
            pass_config.max_spread_depth = config_.max_spread_depth - pass;

            MKActivationConfig old = config_;
            config_ = pass_config;
            MKActivationResult pass_result = spreadActivation(strong_nodes, graph);
            config_ = old;

            // Merge results
            for (const auto& [node, state] : pass_result.full_state) {
                activation_map_[node].energy += state.energy * 0.5f;
                for (const auto& src : state.sources) {
                    activation_map_[node].addActivation(0, src, state.depth_from_source);
                }
            }

            // Refresh top_activated
            std::vector<std::pair<std::string, float>> all;
            for (const auto& [node, state] : activation_map_) {
                all.push_back({node, state.energy});
            }
            std::sort(all.begin(), all.end(),
                     [](const auto& a, const auto& b) { return a.second > b.second; });
            size_t top_count = std::min(all.size(), (size_t)20);
            result.top_activated.assign(all.begin(), all.begin() + top_count);
            if (!all.empty()) result.max_energy = all[0].second;
        }

        return result;
    }
};

#endif // MK_ACTIVATION_ENGINE_CPP
