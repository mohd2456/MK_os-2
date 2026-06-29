#ifndef MK_DEEP_REASONER_CPP
#define MK_DEEP_REASONER_CPP

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <queue>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <functional>

#include "pattern_graph.cpp"

// ===================================================================================
// MK DEEP REASONER — Multi-step complex problem solving
// ===================================================================================
// This is what lets MK solve HARD problems without being an LLM.
//
// Techniques (all invented, no neural networks):
//   1. Chain-of-Thought: Breaks complex queries into sub-questions
//   2. Analogy Engine: Finds structural similarities between concepts
//   3. Hypothesis Testing: Generates possible answers, scores against facts
//   4. Causal Chains: "If X then Y" predictions
//   5. Planning Engine: Breaks goals into ordered executable steps
//   6. Multi-hop Traversal: Walks 10+ edges deep in the knowledge graph
//   7. Contradiction Detection: Flags conflicting information
//
// The key insight: LLMs use statistical patterns. MK uses LOGICAL STRUCTURE.
// With enough knowledge (450GB), structured reasoning beats pattern matching.
// ===================================================================================

// Forward declaration (MKPatternGraph included above)
// class MKPatternGraph;

// A single reasoning step
struct MKThinkStep {
    int stepNum;
    std::string type;           // "decompose", "lookup", "infer", "combine", "verify"
    std::string question;       // What this step is trying to answer
    std::string result;         // What it found
    float confidence;
    std::vector<std::string> evidence;  // Facts that support this step
};

// A complete chain of thought
struct MKThoughtChain {
    std::string originalQuery;
    std::vector<MKThinkStep> steps;
    std::string finalAnswer;
    float overallConfidence;
    int totalHops;
    double thinkTimeMs;
};

// An analogy mapping
struct MKAnalogy {
    std::string sourceA;        // "wing"
    std::string sourceB;        // "bird"
    std::string targetA;        // "fin"
    std::string targetB;        // "fish"
    std::string relation;       // "used_for movement"
    float strength;
};

// A causal chain link
struct MKCausalLink {
    std::string cause;
    std::string effect;
    float probability;          // How likely is effect given cause
    std::string mechanism;      // WHY this happens
};

// A plan step
struct MKPlanStep {
    int order;
    std::string action;
    std::string description;
    std::vector<std::string> requires;  // Prerequisites
    std::vector<std::string> produces;  // What this step creates
    bool completed;
};

class MKDeepReasoner {
private:
    std::vector<MKCausalLink> causalDB;
    std::vector<MKAnalogy> analogyCache;
    int totalThoughts;
    int deepDives;
    int contradictionsFound;
    int maxDepth;

    // Decompose a complex question into sub-questions
    std::vector<std::string> decompose(const std::string& query) {
        std::vector<std::string> subQuestions;
        std::string lower = query;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        
        // Pattern: "how to X" → break into steps
        if (lower.find("how to") != std::string::npos || lower.find("how do") != std::string::npos) {
            std::string topic = lower.substr(lower.find("how") + 7);
            subQuestions.push_back("what is " + topic);
            subQuestions.push_back("what does " + topic + " require");
            subQuestions.push_back("what are the steps for " + topic);
            subQuestions.push_back("what tools are needed for " + topic);
        }
        // Pattern: "why does X Y" → find causes
        else if (lower.find("why") != std::string::npos) {
            std::string topic = lower.substr(lower.find("why") + 4);
            subQuestions.push_back("what causes " + topic);
            subQuestions.push_back("what is " + topic + " related to");
            subQuestions.push_back("what leads to " + topic);
        }
        // Pattern: "compare X and Y" → find similarities and differences
        else if (lower.find("compare") != std::string::npos || lower.find("difference") != std::string::npos) {
            // Extract the two things being compared
            size_t andPos = lower.find(" and ");
            if (andPos != std::string::npos) {
                std::string first = lower.substr(lower.find("compare") + 8, andPos - (lower.find("compare") + 8));
                std::string second = lower.substr(andPos + 5);
                subQuestions.push_back("what is " + first);
                subQuestions.push_back("what is " + second);
                subQuestions.push_back("what does " + first + " have");
                subQuestions.push_back("what does " + second + " have");
            }
        }
        // Pattern: "what if X" → explore consequences
        else if (lower.find("what if") != std::string::npos) {
            std::string scenario = lower.substr(lower.find("what if") + 8);
            subQuestions.push_back("what does " + scenario + " cause");
            subQuestions.push_back("what is affected by " + scenario);
            subQuestions.push_back("what is the opposite of " + scenario);
        }
        // Default: break into what/how/why
        else {
            subQuestions.push_back(query);  // Try direct first
        }
        
        return subQuestions;
    }

public:
    MKDeepReasoner(int maxSearchDepth = 10) 
        : totalThoughts(0), deepDives(0), contradictionsFound(0), maxDepth(maxSearchDepth) {
        std::cout << "[DEEP REASONER] Multi-step reasoning engine ready. Max depth: " << maxDepth << "\n";
        initCausalKnowledge();
    }

    void initCausalKnowledge() {
        // Basic causal chains MK knows
        causalDB.push_back({"heat", "expansion", 0.95f, "thermal energy increases molecular motion"});
        causalDB.push_back({"cold", "contraction", 0.95f, "reduced thermal energy decreases molecular motion"});
        causalDB.push_back({"rain", "wet ground", 0.99f, "water falls from clouds to earth"});
        causalDB.push_back({"exercise", "fitness", 0.85f, "physical stress triggers adaptation"});
        causalDB.push_back({"practice", "skill improvement", 0.9f, "repetition builds neural pathways"});
        causalDB.push_back({"no sleep", "fatigue", 0.95f, "body needs rest to recover"});
        causalDB.push_back({"learning", "knowledge", 0.9f, "new information stored in memory"});
        causalDB.push_back({"code error", "program crash", 0.7f, "invalid operations halt execution"});
        causalDB.push_back({"optimization", "faster code", 0.8f, "fewer operations = less CPU time"});
        causalDB.push_back({"compression", "smaller files", 0.95f, "removes redundant data patterns"});
    }

    // ─────────────────────────────────────────
    //  CHAIN OF THOUGHT — Solve complex queries step by step
    // ─────────────────────────────────────────

    MKThoughtChain think(const std::string& query, MKPatternGraph& graph) {
        totalThoughts++;
        deepDives++;
        MKThoughtChain chain;
        chain.originalQuery = query;
        chain.overallConfidence = 0.0f;
        chain.totalHops = 0;
        
        // Step 1: Decompose the question
        auto subQuestions = decompose(query);
        
        MKThinkStep step1;
        step1.stepNum = 1;
        step1.type = "decompose";
        step1.question = "Break down: " + query;
        step1.result = "Split into " + std::to_string(subQuestions.size()) + " sub-questions";
        step1.confidence = 1.0f;
        chain.steps.push_back(step1);
        
        // Step 2: Try to answer each sub-question from the graph
        std::vector<std::string> findings;
        int stepNum = 2;
        
        for (const auto& subQ : subQuestions) {
            MKThinkStep step;
            step.stepNum = stepNum++;
            step.type = "lookup";
            step.question = subQ;
            
            // Extract keywords and search the graph
            std::stringstream ss(subQ);
            std::string word;
            std::vector<std::string> keywords;
            while (ss >> word) {
                std::transform(word.begin(), word.end(), word.begin(), ::tolower);
                if (word.size() > 3 && word != "what" && word != "does" && 
                    word != "have" && word != "that" && word != "this" && word != "with") {
                    keywords.push_back(word);
                }
            }
            
            // Try graph queries for each keyword
            std::string found;
            for (const auto& kw : keywords) {
                auto results = graph.getAll(kw);
                if (!results.empty()) {
                    for (const auto& edge : results) {
                        found += edge.source + " " + edge.relation + " " + edge.target + ". ";
                        step.evidence.push_back(edge.source + " " + edge.relation + " " + edge.target);
                    }
                }
                // Also try path queries for deeper connections
                for (const auto& otherKw : keywords) {
                    if (otherKw != kw) {
                        auto pathResult = graph.pathQuery(kw, otherKw, maxDepth);
                        if (pathResult.found) {
                            found += kw + " connects to " + pathResult.answer + ". ";
                            chain.totalHops += pathResult.hops;
                        }
                    }
                }
            }
            
            step.result = found.empty() ? "No direct knowledge found." : found;
            step.confidence = found.empty() ? 0.0f : 0.7f;
            chain.steps.push_back(step);
            if (!found.empty()) findings.push_back(found);
        }
        
        // Step 3: Try causal reasoning if direct lookup failed
        if (findings.empty()) {
            MKThinkStep causalStep;
            causalStep.stepNum = stepNum++;
            causalStep.type = "infer";
            causalStep.question = "Check causal knowledge for: " + query;
            
            std::string lower = query;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            
            for (const auto& causal : causalDB) {
                if (lower.find(causal.cause) != std::string::npos) {
                    causalStep.result += causal.cause + " leads to " + causal.effect + 
                                        " (because: " + causal.mechanism + "). ";
                    causalStep.confidence = causal.probability;
                    findings.push_back(causalStep.result);
                }
                if (lower.find(causal.effect) != std::string::npos) {
                    causalStep.result += causal.effect + " is caused by " + causal.cause + 
                                        " (" + causal.mechanism + "). ";
                    causalStep.confidence = causal.probability;
                    findings.push_back(causalStep.result);
                }
            }
            
            if (causalStep.result.empty()) causalStep.result = "No causal links found.";
            chain.steps.push_back(causalStep);
        }
        
        // Step 4: Combine findings into final answer
        MKThinkStep combineStep;
        combineStep.stepNum = stepNum;
        combineStep.type = "combine";
        combineStep.question = "Synthesize all findings";
        
        if (!findings.empty()) {
            std::stringstream answer;
            for (const auto& f : findings) answer << f << " ";
            chain.finalAnswer = answer.str();
            chain.overallConfidence = 0.7f;
            combineStep.result = "Combined " + std::to_string(findings.size()) + " findings.";
            combineStep.confidence = 0.8f;
        } else {
            chain.finalAnswer = "";
            chain.overallConfidence = 0.0f;
            combineStep.result = "Insufficient knowledge to answer.";
            combineStep.confidence = 0.0f;
        }
        chain.steps.push_back(combineStep);
        
        return chain;
    }

    // ─────────────────────────────────────────
    //  PLANNING ENGINE — Break goals into steps
    // ─────────────────────────────────────────

    std::vector<MKPlanStep> plan(const std::string& goal, MKPatternGraph& graph) {
        std::vector<MKPlanStep> steps;
        std::string lower = goal;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        
        // Search graph for knowledge about this goal
        std::stringstream ss(lower);
        std::string word;
        std::vector<std::string> keywords;
        while (ss >> word) {
            if (word.size() > 3) keywords.push_back(word);
        }
        
        // Look for "steps_for", "requires", "needs" relations
        for (const auto& kw : keywords) {
            auto stepsFor = graph.query(kw, "steps_for");
            auto requires = graph.query(kw, "requires");
            auto tools = graph.query(kw, "uses_tool");
            
            int order = 1;
            
            // Add prerequisite steps
            for (const auto& req : requires) {
                MKPlanStep step;
                step.order = order++;
                step.action = "Setup " + req;
                step.description = "Ensure " + req + " is available (required for " + kw + ")";
                step.produces.push_back(req);
                step.completed = false;
                steps.push_back(step);
            }
            
            // Add tool acquisition
            for (const auto& tool : tools) {
                MKPlanStep step;
                step.order = order++;
                step.action = "Get " + tool;
                step.description = "Install or prepare " + tool;
                step.produces.push_back(tool);
                step.completed = false;
                steps.push_back(step);
            }
            
            // Add main execution steps
            for (const auto& s : stepsFor) {
                MKPlanStep step;
                step.order = order++;
                step.action = s;
                step.description = "Execute: " + s;
                step.completed = false;
                steps.push_back(step);
            }
        }
        
        // If no specific knowledge, generate generic plan structure
        if (steps.empty()) {
            steps.push_back({1, "Research", "Gather information about: " + goal, {}, {"knowledge"}, false});
            steps.push_back({2, "Design", "Plan the structure and approach", {"knowledge"}, {"design"}, false});
            steps.push_back({3, "Implement", "Build the core functionality", {"design"}, {"implementation"}, false});
            steps.push_back({4, "Test", "Verify it works correctly", {"implementation"}, {"tested_code"}, false});
            steps.push_back({5, "Refine", "Fix issues and improve", {"tested_code"}, {"final_product"}, false});
        }
        
        return steps;
    }

    // ─────────────────────────────────────────
    //  ANALOGY ENGINE
    // ─────────────────────────────────────────

    // Find analogies: "X is to Y as ? is to Z"
    std::vector<std::string> findAnalogy(const std::string& a, const std::string& b,
                                          const std::string& c, MKPatternGraph& graph) {
        std::vector<std::string> results;
        
        // Find what relation connects A to B
        auto aEdges = graph.getAll(a);
        std::string abRelation;
        for (const auto& edge : aEdges) {
            if (edge.target == b) {
                abRelation = edge.relation;
                break;
            }
        }
        
        if (abRelation.empty()) {
            // Try reverse
            auto bEdges = graph.getAll(b);
            for (const auto& edge : bEdges) {
                if (edge.target == a) {
                    abRelation = edge.relation;
                    break;
                }
            }
        }
        
        if (!abRelation.empty()) {
            // Now find what C has the same relation to
            auto cResults = graph.query(c, abRelation);
            for (const auto& r : cResults) {
                results.push_back(r);
            }
        }
        
        return results;
    }

    // ─────────────────────────────────────────
    //  CONTRADICTION DETECTION
    // ─────────────────────────────────────────

    bool checkContradiction(const std::string& subject, const std::string& relation,
                            const std::string& newValue, MKPatternGraph& graph) {
        auto existing = graph.query(subject, relation);
        
        // Check if the new value conflicts with existing facts
        for (const auto& old : existing) {
            if (old != newValue) {
                // Check if they're known opposites
                if (graph.hasFact(old, "opposite_of", newValue) ||
                    graph.hasFact(newValue, "opposite_of", old)) {
                    contradictionsFound++;
                    std::cout << "[DEEP REASONER] CONTRADICTION: " << subject << " " << relation 
                              << " = \"" << old << "\" vs \"" << newValue << "\"\n";
                    return true;
                }
            }
        }
        return false;
    }

    // ─────────────────────────────────────────
    //  STATS
    // ─────────────────────────────────────────

    void printStats() const {
        std::cout << "\n--- [DEEP REASONER] ---\n";
        std::cout << "  Total thoughts: " << totalThoughts << "\n";
        std::cout << "  Deep dives: " << deepDives << "\n";
        std::cout << "  Causal links: " << causalDB.size() << "\n";
        std::cout << "  Contradictions found: " << contradictionsFound << "\n";
        std::cout << "  Max search depth: " << maxDepth << "\n";
        std::cout << "-----------------------\n";
    }
};

#endif // MK_DEEP_REASONER_CPP
