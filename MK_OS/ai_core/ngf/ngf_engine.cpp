#ifndef MK_NGF_ENGINE_CPP
#define MK_NGF_ENGINE_CPP

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <cmath>

#include "activation_spreading.cpp"
#include "path_tracer.cpp"
#include "verbalizer.cpp"

// ===================================================================================
// MK NEURAL GRAPH FUSION — ENGINE (Orchestrator)
// ===================================================================================
// The main NGF engine that combines:
//   1. Activation Spreading - identifies relevant concept neighborhoods
//   2. Path Tracing - finds optimal reasoning paths between concepts
//   3. Verbalization - converts paths into natural language responses
//
// Pipeline:
//   Query -> Extract Keywords -> Activate Source Nodes -> Spread Activation ->
//   Identify Targets -> Trace Paths -> Score Results -> Verbalize -> Response
//
// This is MK's core "thinking" mechanism for knowledge-based questions.
// ===================================================================================

// Input query to the NGF engine
struct MKNGFQuery {
    std::string raw_query;              // Original natural language query
    std::vector<std::string> keywords;  // Extracted keywords/concepts
    std::string query_type;             // "what", "why", "how", "is", "does", "compare"
    float urgency;                      // How quickly to respond (affects depth)
    int max_depth;                      // Maximum reasoning depth
};

// Result from the NGF engine
struct MKNGFResult {
    bool success;
    std::string answer;                 // Natural language answer
    std::string explanation;            // Reasoning chain explanation
    float confidence;                   // Overall confidence (0-1)
    int facts_used;                     // Number of graph facts involved
    int nodes_activated;                // How many nodes were activated
    int paths_traced;                   // Number of paths explored
    double think_time_ms;               // Processing time in milliseconds
    std::vector<std::string> sources;   // Source concepts
    std::vector<std::string> targets;   // Target concepts reached
};

// NGF engine configuration
struct MKNGFConfig {
    float activation_threshold;     // Minimum activation to consider (default 0.1)
    float path_min_confidence;      // Minimum path confidence to report (default 0.3)
    int max_activated_nodes;        // Cap on spreading (default 100)
    int max_paths_to_trace;         // Maximum paths per query (default 10)
    int top_n_results;              // Top results to verbalize (default 5)
    bool verbose;                   // Print debug info

    MKNGFConfig()
        : activation_threshold(0.1f), path_min_confidence(0.3f),
          max_activated_nodes(100), max_paths_to_trace(10),
          top_n_results(5), verbose(false) {}
};

class MKNGFEngine {
private:
    MKActivationSpreading spreader;
    MKPathTracer tracer;
    MKVerbalizer verbalizer;
    MKNGFConfig config;

    int total_queries;
    int successful_queries;
    double total_think_time_ms;

    // Extract keywords from raw query
    std::vector<std::string> extractKeywords(const std::string& query) {
        std::vector<std::string> keywords;
        std::string lower = query;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        // Remove common stop words
        std::vector<std::string> stop_words = {
            "what", "is", "are", "the", "a", "an", "does", "do", "how",
            "why", "when", "where", "which", "who", "can", "could", "would",
            "should", "will", "shall", "may", "might", "about", "tell", "me",
            "please", "know", "think", "that", "this", "it", "of", "in",
            "on", "at", "to", "for", "with", "from", "by", "and", "or"
        };

        std::stringstream ss(lower);
        std::string word;
        while (ss >> word) {
            // Remove punctuation
            word.erase(std::remove_if(word.begin(), word.end(),
                       [](char c) { return !std::isalnum(static_cast<unsigned char>(c)) && c != '_'; }),
                       word.end());
            
            if (word.size() < 2) continue;
            
            bool is_stop = false;
            for (const auto& sw : stop_words) {
                if (word == sw) { is_stop = true; break; }
            }
            if (!is_stop) {
                keywords.push_back(word);
            }
        }
        return keywords;
    }

    // Determine query type from the raw question
    std::string classifyQuery(const std::string& query) {
        std::string lower = query;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        if (lower.find("what") != std::string::npos) return "what";
        if (lower.find("why") != std::string::npos) return "why";
        if (lower.find("how") != std::string::npos) return "how";
        if (lower.find("compare") != std::string::npos || 
            lower.find("difference") != std::string::npos) return "compare";
        if (lower.find("is ") == 0 || lower.find("does ") == 0 ||
            lower.find("can ") == 0) return "yes_no";
        if (lower.find("where") != std::string::npos) return "where";
        if (lower.find("who") != std::string::npos) return "who";
        return "general";
    }

    // Build NGF query struct from raw text
    MKNGFQuery buildQuery(const std::string& raw_query) {
        MKNGFQuery q;
        q.raw_query = raw_query;
        q.keywords = extractKeywords(raw_query);
        q.query_type = classifyQuery(raw_query);
        q.urgency = 0.5f;
        q.max_depth = 6;
        return q;
    }

public:
    MKNGFEngine() : total_queries(0), successful_queries(0), total_think_time_ms(0.0) {
        std::cout << "[NGF ENGINE] Neural Graph Fusion engine initialized.\n";
        std::cout << "  Modules: Activation Spreading + Path Tracing + Verbalization\n";
    }

    MKNGFEngine(const MKNGFConfig& cfg) 
        : config(cfg), total_queries(0), successful_queries(0), total_think_time_ms(0.0) {
        std::cout << "[NGF ENGINE] Initialized with custom config.\n";
    }

    // ─────────────────────────────────────────
    //  GRAPH LOADING
    // ─────────────────────────────────────────

    // Load knowledge edges into the NGF engine (both spreader and tracer)
    void loadEdge(const std::string& source, const std::string& relation,
                  const std::string& target, float weight = 1.0f) {
        spreader.addEdge(source, relation, target, weight);
        tracer.addEdge(source, relation, target, weight);
    }

    // Bulk load from a vector of tuples
    void loadKnowledge(const std::vector<std::vector<std::string>>& facts) {
        for (const auto& fact : facts) {
            if (fact.size() >= 3) {
                float w = (fact.size() >= 4) ? std::stof(fact[3]) : 1.0f;
                loadEdge(fact[0], fact[1], fact[2], w);
            }
        }
        if (config.verbose) {
            std::cout << "[NGF ENGINE] Loaded " << facts.size() << " facts.\n";
        }
    }

    // ─────────────────────────────────────────
    //  CORE: THINK (Main reasoning pipeline)
    // ─────────────────────────────────────────

    MKNGFResult think(const std::string& raw_query) {
        auto start_time = std::chrono::high_resolution_clock::now();
        total_queries++;

        MKNGFResult result;
        result.success = false;
        result.confidence = 0.0f;
        result.facts_used = 0;
        result.nodes_activated = 0;
        result.paths_traced = 0;

        // Step 1: Parse the query
        MKNGFQuery query = buildQuery(raw_query);
        
        if (query.keywords.empty()) {
            result.answer = "I need more specific terms to search my knowledge graph.";
            result.confidence = 0.0f;
            auto end_time = std::chrono::high_resolution_clock::now();
            result.think_time_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
            return result;
        }

        if (config.verbose) {
            std::cout << "[NGF] Query type: " << query.query_type << "\n";
            std::cout << "[NGF] Keywords: ";
            for (const auto& kw : query.keywords) std::cout << kw << " ";
            std::cout << "\n";
        }

        // Step 2: Spread activation from all keyword nodes
        std::vector<std::pair<std::string, float>> activation_sources;
        for (const auto& kw : query.keywords) {
            activation_sources.push_back({kw, 1.0f});
        }
        
        MKSpreadResult spread_result = spreader.spreadMulti(activation_sources);
        result.nodes_activated = spread_result.nodes_activated;
        result.sources = query.keywords;

        if (config.verbose) {
            std::cout << "[NGF] Activated " << spread_result.nodes_activated << " nodes in "
                      << spread_result.ticks_elapsed << " ticks.\n";
        }

        // Step 3: Get top activated nodes as potential targets
        auto top_nodes = spreader.getTopActivated(spread_result, config.max_activated_nodes);
        
        // Filter out source nodes from targets
        std::vector<MKActivationState> target_nodes;
        for (const auto& node : top_nodes) {
            if (!node.is_source && node.energy >= config.activation_threshold) {
                target_nodes.push_back(node);
            }
        }

        // Step 4: Trace paths from sources to top targets
        std::vector<MKTracedPath> all_paths;
        for (const auto& source_kw : query.keywords) {
            int paths_traced = 0;
            for (const auto& target : target_nodes) {
                if (paths_traced >= config.max_paths_to_trace) break;
                
                MKTraceResult trace = tracer.trace(source_kw, target.node_name);
                if (trace.found) {
                    for (auto& p : trace.paths) {
                        if (p.total_weight >= config.path_min_confidence) {
                            all_paths.push_back(p);
                        }
                    }
                }
                paths_traced++;
            }
        }
        result.paths_traced = static_cast<int>(all_paths.size());

        // Step 5: Sort paths by score and select top results
        std::sort(all_paths.begin(), all_paths.end(),
                  [](const MKTracedPath& a, const MKTracedPath& b) {
                      return a.path_score > b.path_score;
                  });

        if (static_cast<int>(all_paths.size()) > config.top_n_results) {
            all_paths.resize(config.top_n_results);
        }

        // Step 6: Verbalize the results
        if (!all_paths.empty()) {
            result.success = true;
            successful_queries++;

            // Convert paths to verbalizer inputs
            std::vector<MKVerbalizerInput> verb_inputs;
            for (const auto& path : all_paths) {
                for (const auto& hop : path.hops) {
                    MKVerbalizerInput vi;
                    vi.subject = hop.from_node;
                    vi.relation = hop.relation;
                    vi.object = hop.to_node;
                    vi.weight = hop.edge_weight;
                    vi.hop_number = hop.hop_number;
                    verb_inputs.push_back(vi);
                }
            }
            result.facts_used = static_cast<int>(verb_inputs.size());

            // Verbalize
            MKParagraph para = verbalizer.verbalizePath(verb_inputs);
            result.answer = para.assembled_text;
            result.confidence = para.avg_confidence;

            // Generate explanation if it is a "why" query
            if (query.query_type == "why" && !all_paths.empty()) {
                std::vector<MKVerbalizerInput> best_path_inputs;
                for (const auto& hop : all_paths[0].hops) {
                    MKVerbalizerInput vi;
                    vi.subject = hop.from_node;
                    vi.relation = hop.relation;
                    vi.object = hop.to_node;
                    vi.weight = hop.edge_weight;
                    vi.hop_number = hop.hop_number;
                    best_path_inputs.push_back(vi);
                }
                result.explanation = verbalizer.explainPath(
                    all_paths[0].source, all_paths[0].target, best_path_inputs);
            }

            // Collect target concepts
            for (const auto& path : all_paths) {
                result.targets.push_back(path.target);
            }
        } else {
            result.answer = "I could not find a strong knowledge path for this query.";
            result.confidence = 0.0f;
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        result.think_time_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        total_think_time_ms += result.think_time_ms;

        if (config.verbose) {
            std::cout << "[NGF] Result: " << (result.success ? "SUCCESS" : "NO_RESULT")
                      << " confidence=" << result.confidence
                      << " time=" << result.think_time_ms << "ms\n";
        }

        return result;
    }

    // ─────────────────────────────────────────
    //  CONVENIENCE METHODS
    // ─────────────────────────────────────────

    // Quick query with just a string response
    std::string ask(const std::string& query) {
        MKNGFResult result = think(query);
        return result.answer;
    }

    // Compare two concepts
    std::string compare(const std::string& conceptA, const std::string& conceptB) {
        // Trace paths between the two concepts
        MKTraceResult forward = tracer.trace(conceptA, conceptB);
        MKTraceResult backward = tracer.trace(conceptB, conceptA);

        std::string comparison;
        if (forward.found) {
            comparison += "From " + conceptA + " to " + conceptB + ": " + forward.best_summary + ". ";
        }
        if (backward.found) {
            comparison += "From " + conceptB + " to " + conceptA + ": " + backward.best_summary + ". ";
        }
        if (comparison.empty()) {
            comparison = "I cannot find a direct connection between " + conceptA + " and " + conceptB + ".";
        }
        return comparison;
    }

    // ─────────────────────────────────────────
    //  CONFIGURATION
    // ─────────────────────────────────────────

    void setConfig(const MKNGFConfig& cfg) { config = cfg; }
    MKNGFConfig getConfig() const { return config; }
    void setVerbose(bool v) { config.verbose = v; }

    // ─────────────────────────────────────────
    //  STATS
    // ─────────────────────────────────────────

    void printStats() const {
        std::cout << "\n=== [NGF ENGINE STATS] ===\n";
        std::cout << "  Total queries: " << total_queries << "\n";
        std::cout << "  Successful: " << successful_queries << "\n";
        std::cout << "  Success rate: " 
                  << (total_queries > 0 ? (100.0f * successful_queries / total_queries) : 0.0f) 
                  << "%\n";
        std::cout << "  Avg think time: " 
                  << (total_queries > 0 ? (total_think_time_ms / total_queries) : 0.0) << "ms\n";
        std::cout << "=========================\n";
        spreader.printStats();
        tracer.printStats();
        verbalizer.printStats();
    }
};

#endif // MK_NGF_ENGINE_CPP
