#ifndef MK_NGF_MAIN_CPP
#define MK_NGF_MAIN_CPP

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <chrono>
#include <functional>

#include "concept_web.cpp"
#include "activation_engine.cpp"
#include "path_tracer.cpp"
#include "verbalizer.cpp"
#include "growth_engine.cpp"

// ===================================================================================
// MK NEURAL GRAPH FUSION - MAIN COORDINATOR
// ===================================================================================
// The main NGF coordinator. Takes a query string and runs it through the full
// pipeline: shatter -> activate -> trace -> verbalize. Returns the answer,
// confidence score, and reasoning path.
//
// Pipeline:
//   1. SHATTER: Break query into concept atoms (individual concepts)
//   2. ACTIVATE: Spread activation energy from atoms through the concept web
//   3. TRACE: Find strongest paths between high-activation zones
//   4. VERBALIZE: Convert reasoning paths into natural English
//
// This is the unified interface for the NGF engine. Other modules call think()
// and get back a complete answer with confidence and explanation.
// ===================================================================================

// Complete result from the NGF engine
struct MKNGFMainResult {
    bool success;
    std::string answer;                 // Natural language answer
    std::string reasoning;              // Explanation of reasoning chain
    std::string short_answer;           // One-line summary
    float confidence;                   // Overall confidence (0-1)
    int concepts_activated;             // Number of concepts that lit up
    int paths_found;                    // Number of reasoning paths found
    int facts_used;                     // Number of facts in the reasoning
    double total_time_ms;               // Total processing time
    std::vector<std::string> source_concepts;  // Input concepts identified
    std::vector<std::string> answer_concepts;  // Key concepts in the answer
    std::vector<std::string> reasoning_chain;  // Step-by-step reasoning

    MKNGFMainResult()
        : success(false), confidence(0.0f), concepts_activated(0),
          paths_found(0), facts_used(0), total_time_ms(0.0) {}
};

// Query type classification
enum class MKQueryType {
    WHAT,       // "What is X?"
    WHY,        // "Why does X?"
    HOW,        // "How does X work?"
    IS,         // "Is X a Y?"
    DOES,       // "Does X cause Y?"
    COMPARE,    // "Compare X and Y"
    RELATE,     // "How are X and Y related?"
    LIST,       // "List types of X"
    DEFINE,     // "Define X"
    UNKNOWN
};

// The main NGF coordinator class
class MKNGFMain {
private:
    MKConceptWeb concept_web_;
    MKActivationEngine activation_;
    MKPathTracer tracer_;
    MKVerbalizer verbalizer_;
    MKGrowthEngine growth_;

    // Statistics
    size_t total_queries_;
    size_t successful_queries_;
    double total_time_ms_;

    // Classify the query type
    MKQueryType classifyQuery(const std::string& query) const {
        std::string lower = query;
        std::transform(lower.begin(), lower.end(), lower.begin(),
                      [](unsigned char c) { return std::tolower(c); });

        if (lower.find("what is") != std::string::npos ||
            lower.find("what are") != std::string::npos) return MKQueryType::WHAT;
        if (lower.find("why") != std::string::npos) return MKQueryType::WHY;
        if (lower.find("how") != std::string::npos) return MKQueryType::HOW;
        if (lower.substr(0, 2) == "is" || lower.find(" is ") != std::string::npos)
            return MKQueryType::IS;
        if (lower.find("does") != std::string::npos) return MKQueryType::DOES;
        if (lower.find("compare") != std::string::npos ||
            lower.find("difference") != std::string::npos) return MKQueryType::COMPARE;
        if (lower.find("relate") != std::string::npos ||
            lower.find("relationship") != std::string::npos ||
            lower.find("connection") != std::string::npos) return MKQueryType::RELATE;
        if (lower.find("list") != std::string::npos ||
            lower.find("types of") != std::string::npos) return MKQueryType::LIST;
        if (lower.find("define") != std::string::npos ||
            lower.find("meaning") != std::string::npos) return MKQueryType::DEFINE;

        return MKQueryType::UNKNOWN;
    }

    // Extract the main subject from a query
    std::string extractSubject(const std::string& query) const {
        // Simple extraction: take the last significant noun phrase
        std::vector<std::string> atoms = activation_.shatter(query);
        if (!atoms.empty()) return atoms[0];
        return query;
    }

    // Build the graph structure from concept web for activation engine
    std::unordered_map<std::string, std::vector<std::pair<std::string, float>>>
    buildActivationGraph() {
        std::unordered_map<std::string, std::vector<std::pair<std::string, float>>> graph;

        // Get graph from growth engine
        graph = growth_.exportAsGraph();

        // Also include concept web edges
        auto all_nodes = concept_web_.getAllNodes();
        for (const auto& node : all_nodes) {
            auto edges = concept_web_.getEdgesFrom(node);
            for (const auto& edge : edges) {
                graph[node].push_back({edge.target, edge.weight * edge.confidence});
            }
        }

        return graph;
    }

    // Build edge relations map for path tracer
    std::unordered_map<std::string, std::string> buildEdgeRelations() {
        auto relations = growth_.exportEdgeRelations();

        // Add concept web relations
        auto all_nodes = concept_web_.getAllNodes();
        for (const auto& node : all_nodes) {
            auto edges = concept_web_.getEdgesFrom(node);
            for (const auto& edge : edges) {
                std::string key = node + "|" + edge.target;
                relations[key] = concept_web_.getRelationName(edge.relation);
            }
        }

        return relations;
    }

    // Format answer based on query type
    std::string formatAnswer(MKQueryType type, const std::string& subject,
                            const MKVerbalOutput& verbal, float confidence) {
        std::ostringstream oss;

        switch (type) {
            case MKQueryType::WHAT:
            case MKQueryType::DEFINE:
                if (!verbal.full_text.empty()) {
                    oss << verbal.full_text;
                } else {
                    oss << "Based on my knowledge, " << subject
                        << " is a concept I can reason about, though I may need "
                        << "more specific information to give a complete answer.";
                }
                break;

            case MKQueryType::WHY:
                if (!verbal.full_text.empty()) {
                    oss << "The reason is: " << verbal.full_text;
                } else {
                    oss << "I could not find a clear causal explanation for this.";
                }
                break;

            case MKQueryType::HOW:
                if (!verbal.full_text.empty()) {
                    oss << "Here is how it works: " << verbal.full_text;
                } else {
                    oss << "I need more information to explain the mechanism.";
                }
                break;

            case MKQueryType::IS:
            case MKQueryType::DOES:
                if (confidence > 0.7f) {
                    oss << "Yes, " << verbal.full_text;
                } else if (confidence > 0.4f) {
                    oss << "It appears so. " << verbal.full_text;
                } else {
                    oss << "I'm not certain. " << verbal.full_text;
                }
                break;

            case MKQueryType::COMPARE:
                if (!verbal.full_text.empty()) {
                    oss << "Comparing these concepts: " << verbal.full_text;
                } else {
                    oss << "I could not find enough information to compare these.";
                }
                break;

            case MKQueryType::RELATE:
                if (!verbal.full_text.empty()) {
                    oss << "These concepts are connected: " << verbal.full_text;
                } else {
                    oss << "I could not find a clear connection between these concepts.";
                }
                break;

            case MKQueryType::LIST:
                if (!verbal.full_text.empty()) {
                    oss << verbal.full_text;
                } else {
                    oss << "I could not generate a list for this query.";
                }
                break;

            default:
                if (!verbal.full_text.empty()) {
                    oss << verbal.full_text;
                } else {
                    oss << "I processed your query but could not generate a clear answer. "
                        << "Try rephrasing or asking about a more specific concept.";
                }
                break;
        }

        return oss.str();
    }

public:
    MKNGFMain() : total_queries_(0), successful_queries_(0), total_time_ms_(0.0) {
        std::cout << "[NGF Main] Neural Graph Fusion engine initialized" << std::endl;
        std::cout << "[NGF Main] Pipeline: SHATTER -> ACTIVATE -> TRACE -> VERBALIZE" << std::endl;
    }

    MKNGFMain(const std::string& knowledge_path)
        : growth_(knowledge_path), total_queries_(0),
          successful_queries_(0), total_time_ms_(0.0) {
        std::cout << "[NGF Main] Initialized with knowledge path: " << knowledge_path << std::endl;
    }

    // =========================================================================
    // MAIN THINK PIPELINE
    // =========================================================================

    // The core reasoning function: takes a query, returns a complete answer
    MKNGFMainResult think(const std::string& query) {
        auto start_time = std::chrono::high_resolution_clock::now();
        MKNGFMainResult result;
        total_queries_++;

        if (query.empty()) {
            result.answer = "Please provide a question or topic to reason about.";
            return result;
        }

        // --- STEP 1: SHATTER ---
        // Break the query into concept atoms
        std::vector<std::string> atoms = activation_.shatter(query);
        result.source_concepts = atoms;

        if (atoms.empty()) {
            result.answer = "I could not identify any concepts in your query.";
            return result;
        }

        // Classify query type
        MKQueryType qtype = classifyQuery(query);

        // --- STEP 2: ACTIVATE ---
        // Build graph and spread activation from source concepts
        auto graph = buildActivationGraph();
        MKActivationResult activation_result = activation_.spreadActivation(atoms, graph);

        result.concepts_activated = activation_result.total_nodes_activated;

        if (!activation_result.success || activation_result.top_activated.empty()) {
            // No activation found - concepts not in graph
            result.answer = "I don't have enough knowledge about '" + query +
                          "' in my concept graph to reason about it.";
            result.confidence = 0.1f;
            auto end_time = std::chrono::high_resolution_clock::now();
            result.total_time_ms = std::chrono::duration<double, std::milli>(
                end_time - start_time).count();
            return result;
        }

        // --- STEP 3: TRACE ---
        // Find reasoning paths between source concepts and high-activation zones
        std::vector<std::string> targets;
        for (const auto& [node, energy] : activation_result.top_activated) {
            // Skip source concepts as targets
            bool is_source = false;
            for (const auto& atom : atoms) {
                if (atom == node) { is_source = true; break; }
            }
            if (!is_source && energy > activation_result.avg_energy) {
                targets.push_back(node);
            }
            if (targets.size() >= 5) break;
        }

        // Also include convergence points as high-priority targets
        for (const auto& cp : activation_result.convergence_points) {
            if (std::find(targets.begin(), targets.end(), cp) == targets.end()) {
                targets.push_back(cp);
            }
            if (targets.size() >= 8) break;
        }

        // Build energy map for path scoring
        std::unordered_map<std::string, float> energy_map;
        for (const auto& [node, state] : activation_result.full_state) {
            energy_map[node] = state.energy;
        }

        auto edge_relations = buildEdgeRelations();
        MKTraceResult trace_result = tracer_.traceMultiple(
            atoms, targets, energy_map, graph, edge_relations);

        result.paths_found = (int)trace_result.paths.size();

        // --- STEP 4: VERBALIZE ---
        MKVerbalOutput verbal;

        if (trace_result.success && !trace_result.paths.empty()) {
            // Verbalize the best paths
            std::vector<std::vector<std::string>> all_concepts;
            std::vector<std::vector<std::string>> all_relations;
            std::vector<std::vector<float>> all_confidences;

            int paths_to_verbalize = std::min((int)trace_result.paths.size(), 3);
            for (int i = 0; i < paths_to_verbalize; i++) {
                const auto& path = trace_result.paths[i];
                std::vector<std::string> concepts;
                std::vector<std::string> relations;
                std::vector<float> confidences;

                for (const auto& step : path.steps) {
                    concepts.push_back(step.concept);
                    if (!step.relation_to_next.empty()) {
                        relations.push_back(step.relation_to_next);
                        confidences.push_back(step.edge_weight);
                    }
                }
                if (!concepts.empty()) {
                    all_concepts.push_back(concepts);
                    all_relations.push_back(relations);
                    all_confidences.push_back(confidences);
                }
            }

            verbal = verbalizer_.verbalizeMultiplePaths(all_concepts, all_relations, all_confidences);
        }

        // Format the final answer
        std::string subject = extractSubject(query);
        float confidence = trace_result.success ?
            trace_result.overall_confidence : 0.2f;

        result.answer = formatAnswer(qtype, subject, verbal, confidence);
        result.reasoning = verbal.reasoning_summary;
        result.short_answer = verbal.short_answer;
        result.confidence = confidence;
        result.facts_used = verbal.facts_used;
        result.success = trace_result.success;

        // Build reasoning chain
        if (trace_result.success && !trace_result.best_path.steps.empty()) {
            for (const auto& step : trace_result.best_path.steps) {
                result.reasoning_chain.push_back(step.concept);
            }
        }

        // Collect answer concepts
        for (const auto& t : targets) {
            result.answer_concepts.push_back(t);
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        result.total_time_ms = std::chrono::duration<double, std::milli>(
            end_time - start_time).count();
        total_time_ms_ += result.total_time_ms;

        if (result.success) successful_queries_++;

        return result;
    }

    // =========================================================================
    // KNOWLEDGE MANAGEMENT
    // =========================================================================

    // Add a fact to the knowledge graph
    bool addFact(const std::string& source, const std::string& relation,
                 const std::string& target, float weight = 0.7f) {
        concept_web_.addEdge(source, target, relation, weight);
        growth_.learnFromConversation(source, relation, target, weight);
        return true;
    }

    // Load knowledge from a file
    size_t loadKnowledge(const std::string& filepath) {
        size_t web_loaded = concept_web_.loadFromFile(filepath);
        size_t growth_loaded = growth_.loadFromFile(filepath);
        return std::max(web_loaded, growth_loaded);
    }

    // Save knowledge to disk
    bool saveKnowledge() {
        return growth_.persistToDisk();
    }

    // Restore knowledge from disk
    bool restoreKnowledge() {
        return growth_.loadFromDisk();
    }

    // =========================================================================
    // ACCESS TO SUB-ENGINES
    // =========================================================================

    MKConceptWeb& getConceptWeb() { return concept_web_; }
    MKActivationEngine& getActivation() { return activation_; }
    MKPathTracer& getPathTracer() { return tracer_; }
    MKVerbalizer& getVerbalizer() { return verbalizer_; }
    MKGrowthEngine& getGrowth() { return growth_; }

    // =========================================================================
    // STATISTICS AND DIAGNOSTICS
    // =========================================================================

    void printStats() const {
        std::cout << "\n[NGF Main] === Engine Statistics ===" << std::endl;
        std::cout << "  Total queries: " << total_queries_ << std::endl;
        std::cout << "  Successful: " << successful_queries_ << std::endl;
        float success_rate = total_queries_ > 0 ?
            (float)successful_queries_ / total_queries_ * 100.0f : 0.0f;
        std::cout << "  Success rate: " << success_rate << "%" << std::endl;
        std::cout << "  Avg time: " << (total_queries_ > 0 ? total_time_ms_ / total_queries_ : 0)
                  << " ms" << std::endl;
        std::cout << "\n  Sub-engine stats:" << std::endl;
        concept_web_.printStats();
        growth_.printStats();
        verbalizer_.printStats();
    }

    size_t getTotalQueries() const { return total_queries_; }
    size_t getSuccessfulQueries() const { return successful_queries_; }
    float getSuccessRate() const {
        return total_queries_ > 0 ? (float)successful_queries_ / total_queries_ : 0.0f;
    }
    double getAverageTime() const {
        return total_queries_ > 0 ? total_time_ms_ / total_queries_ : 0.0;
    }
};

#endif // MK_NGF_MAIN_CPP
