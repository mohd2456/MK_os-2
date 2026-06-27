// ============================================================================
// MK OS — Adaptive Reasoning Depth
// ============================================================================
// MK decides HOW HARD to think based on the question.
//
// Levels:
//   Level 0: Heuristic bypass (time, status) → <1ms
//   Level 1: Direct lookup (single fact) → ~2ms
//   Level 2: Multi-hop (2-3 edges) → ~10ms
//   Level 3: Reasoning chains (apply rules) → ~50ms
//   Level 4: Deep think (decompose + multi-query) → ~200ms
//   Level 5: Full analysis (hypothesis testing + planning) → ~1s
//
// Progressive deepening: if Level 1 finds good answer, STOP.
// Thermal-aware: if Mac is hot, cap at Level 2.
// Tracks which level succeeds for different query types.
//
// Pure C++ STL. No external dependencies.
// ============================================================================

#ifndef MK_ADAPTIVE_DEPTH_CPP
#define MK_ADAPTIVE_DEPTH_CPP

#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <chrono>

// ============================================================================
// Constants
// ============================================================================

static const int MAX_DEPTH_LEVEL = 5;
static const float HIGH_CONFIDENCE_THRESHOLD = 0.8f;
static const float MEDIUM_CONFIDENCE_THRESHOLD = 0.5f;

// Time budgets per level (milliseconds)
static const int TIME_BUDGET[] = {1, 2, 10, 50, 200, 1000};

// ============================================================================
// Structures
// ============================================================================

// Result from adaptive thinking
struct ThinkResult {
    std::string answer;           // The answer found
    float confidence;             // How confident (0-1)
    int depthUsed;               // Which level found the answer
    int timeMs;                  // How long it took
    std::string reasoning;       // Explanation of reasoning path
    bool fromCache;              // Was this a cached result?
};

// Query classification for complexity assessment
struct QueryProfile {
    int conceptCount;            // Number of concepts mentioned
    bool hasWhy;                 // "why" questions are harder
    bool hasHow;                 // "how" questions need explanation
    bool hasCompare;             // Comparisons need multi-lookup
    bool hasExplain;             // Explanations need deep reasoning
    bool hasHypothetical;        // "what if" needs simulation
    bool isFactual;              // Simple fact lookup
    float domainFamiliarity;    // How well MK knows this domain (0-1)
    std::string primaryConcept; // Main thing being asked about
};

// Statistics per query type — what depth usually works
struct DepthStats {
    int attempts;                // Times this type was asked
    int successAtLevel[6];      // How often each level succeeded
    float avgConfidence[6];     // Average confidence per level
    int optimalLevel;            // Most efficient level for this type
};

// Graph edge for queries
struct DepthGraphEdge {
    std::string source;
    std::string relation;
    std::string target;
    float weight;
};


// ============================================================================
// MKAdaptiveDepth — Main Class
// ============================================================================

class MKAdaptiveDepth {
private:
    // Maximum allowed depth (can be capped by thermal governor)
    int maxAllowedDepth;

    // Current thermal state (0 = cool, 100 = hot)
    int thermalLevel;

    // Statistics for each query type (for learning optimal depth)
    std::map<std::string, DepthStats> queryStats;

    // Complexity signal keywords
    std::set<std::string> easySignals;
    std::set<std::string> hardSignals;
    std::set<std::string> compareSignals;
    std::set<std::string> explainSignals;
    std::set<std::string> hypotheticalSignals;

    // Heuristic bypass patterns (Level 0 — instant answers)
    std::map<std::string, std::string> heuristicPatterns;

    // Known domains and familiarity scores
    std::map<std::string, float> domainFamiliarity;

    // ========================================================================
    // Private: Initialization
    // ========================================================================

    void initSignals() {
        easySignals = {"what", "who", "when", "where", "name", "list", "define"};
        hardSignals = {"why", "how", "explain", "analyze", "evaluate", "prove"};
        compareSignals = {"compare", "difference", "versus", "vs", "better",
                         "similar", "unlike", "contrast"};
        explainSignals = {"explain", "describe", "elaborate", "detail",
                         "tell me about", "walk me through"};
        hypotheticalSignals = {"what if", "suppose", "imagine", "hypothetically",
                              "could", "would", "might"};
    }

    void initHeuristics() {
        // Instant answers — no graph lookup needed
        heuristicPatterns["what time"] = "[SYSTEM_TIME]";
        heuristicPatterns["hello"] = "[GREETING]";
        heuristicPatterns["hi"] = "[GREETING]";
        heuristicPatterns["hey"] = "[GREETING]";
        heuristicPatterns["thanks"] = "[ACKNOWLEDGMENT]";
        heuristicPatterns["thank you"] = "[ACKNOWLEDGMENT]";
        heuristicPatterns["goodbye"] = "[FAREWELL]";
        heuristicPatterns["bye"] = "[FAREWELL]";
        heuristicPatterns["how are you"] = "[STATUS_REPORT]";
        heuristicPatterns["status"] = "[STATUS_REPORT]";
    }

    // ========================================================================
    // Private: Query Profiling
    // ========================================================================

    QueryProfile profileQuery(const std::string& query) {
        QueryProfile profile;
        profile.conceptCount = 0;
        profile.hasWhy = false;
        profile.hasHow = false;
        profile.hasCompare = false;
        profile.hasExplain = false;
        profile.hasHypothetical = false;
        profile.isFactual = false;
        profile.domainFamiliarity = 0.5f;

        // Tokenize
        std::istringstream stream(query);
        std::string word;
        std::vector<std::string> words;
        while (stream >> word) {
            std::string lower = word;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            words.push_back(lower);
        }

        // Count content words (non-stopwords = concepts)
        std::set<std::string> stopwords = {
            "is", "a", "an", "the", "of", "in", "to", "for", "on", "with",
            "it", "that", "this", "are", "was", "be", "do", "does", "did"
        };
        for (const auto& w : words) {
            if (stopwords.count(w) == 0) {
                profile.conceptCount++;
                if (profile.primaryConcept.empty()) {
                    profile.primaryConcept = w;
                }
            }
        }

        // Detect signals
        std::string queryLower = query;
        std::transform(queryLower.begin(), queryLower.end(),
                      queryLower.begin(), ::tolower);

        for (const auto& s : hardSignals) {
            if (queryLower.find(s) != std::string::npos) {
                if (s == "why") profile.hasWhy = true;
                if (s == "how") profile.hasHow = true;
            }
        }
        for (const auto& s : compareSignals) {
            if (queryLower.find(s) != std::string::npos) profile.hasCompare = true;
        }
        for (const auto& s : explainSignals) {
            if (queryLower.find(s) != std::string::npos) profile.hasExplain = true;
        }
        for (const auto& s : hypotheticalSignals) {
            if (queryLower.find(s) != std::string::npos) profile.hasHypothetical = true;
        }

        // Simple factual if starts with "what is" and short
        if (queryLower.find("what is") == 0 && words.size() <= 5) {
            profile.isFactual = true;
        }

        // Check domain familiarity
        for (const auto& pair : domainFamiliarity) {
            if (queryLower.find(pair.first) != std::string::npos) {
                profile.domainFamiliarity = pair.second;
                break;
            }
        }

        return profile;
    }


public:
    // ========================================================================
    // Constructor
    // ========================================================================

    MKAdaptiveDepth() : maxAllowedDepth(MAX_DEPTH_LEVEL), thermalLevel(0) {
        initSignals();
        initHeuristics();

        // Default domain familiarity
        domainFamiliarity["programming"] = 0.9f;
        domainFamiliarity["code"] = 0.9f;
        domainFamiliarity["computer"] = 0.8f;
        domainFamiliarity["science"] = 0.7f;
        domainFamiliarity["math"] = 0.7f;
        domainFamiliarity["animal"] = 0.6f;
        domainFamiliarity["history"] = 0.5f;
        domainFamiliarity["philosophy"] = 0.4f;
    }

    // ========================================================================
    // Core API: Assess Complexity
    // ========================================================================

    // Score 0-5 based on query analysis
    int assessComplexity(const std::string& query) {
        QueryProfile profile = profileQuery(query);

        // Level 0: Heuristic bypass (greetings, time, status)
        std::string queryLower = query;
        std::transform(queryLower.begin(), queryLower.end(),
                      queryLower.begin(), ::tolower);
        for (const auto& pair : heuristicPatterns) {
            if (queryLower.find(pair.first) != std::string::npos) {
                return 0;
            }
        }

        // Level 1: Simple factual (1 concept, no complex signals)
        if (profile.isFactual && profile.conceptCount <= 2 &&
            !profile.hasWhy && !profile.hasCompare) {
            return 1;
        }

        // Level 2: Multi-hop (2-3 concepts, needs chaining)
        if (profile.conceptCount <= 3 && !profile.hasWhy &&
            !profile.hasCompare && !profile.hasHypothetical) {
            return 2;
        }

        // Level 3: Reasoning chains (why/how questions about known domains)
        if ((profile.hasWhy || profile.hasHow) && profile.domainFamiliarity > 0.6f &&
            !profile.hasCompare && !profile.hasHypothetical) {
            return 3;
        }

        // Level 4: Deep think (comparisons, explanations, unfamiliar)
        if (profile.hasCompare || profile.hasExplain ||
            profile.domainFamiliarity < 0.4f) {
            return 4;
        }

        // Level 5: Full analysis (hypotheticals, multi-concept + why)
        if (profile.hasHypothetical || (profile.hasWhy && profile.conceptCount > 3)) {
            return 5;
        }

        // Default: base on concept count
        return std::min(profile.conceptCount / 2, 3);
    }

    // ========================================================================
    // Core API: Set Max Depth
    // ========================================================================

    // Cap reasoning depth (for thermal throttling or user preference)
    void setMaxDepth(int level) {
        maxAllowedDepth = std::max(0, std::min(level, MAX_DEPTH_LEVEL));
        std::cout << "[AdaptiveDepth] Max depth set to " << maxAllowedDepth << std::endl;
    }

    // ========================================================================
    // Core API: Update Thermal
    // ========================================================================

    // Called by thermal governor — adjusts max depth based on temperature
    void updateThermal(int temperature) {
        thermalLevel = temperature;

        // Thermal throttling:
        // < 50°C: full depth (5)
        // 50-65°C: cap at 4
        // 65-75°C: cap at 3
        // 75-85°C: cap at 2
        // > 85°C: cap at 1 (emergency — only direct lookups)
        if (temperature < 50) {
            maxAllowedDepth = 5;
        } else if (temperature < 65) {
            maxAllowedDepth = 4;
        } else if (temperature < 75) {
            maxAllowedDepth = 3;
        } else if (temperature < 85) {
            maxAllowedDepth = 2;
        } else {
            maxAllowedDepth = 1;
            std::cout << "[AdaptiveDepth] THERMAL WARNING: Depth capped at 1!" << std::endl;
        }
    }


    // ========================================================================
    // Core API: Think Adaptive
    // ========================================================================

    // The main entry point: automatically picks right depth and searches
    // Uses progressive deepening — stops as soon as confidence is high enough
    ThinkResult thinkAdaptive(const std::string& query,
                              const std::vector<DepthGraphEdge>& graph) {
        ThinkResult result;
        result.confidence = 0.0f;
        result.depthUsed = 0;
        result.fromCache = false;

        auto startTime = std::chrono::high_resolution_clock::now();

        // Determine target depth
        int targetDepth = assessComplexity(query);
        int effectiveDepth = std::min(targetDepth, maxAllowedDepth);

        // Check learned optimal depth for this query type
        QueryProfile profile = profileQuery(query);
        auto statsIt = queryStats.find(profile.primaryConcept);
        if (statsIt != queryStats.end() && statsIt->second.optimalLevel > 0) {
            // We've learned the optimal depth for this type
            effectiveDepth = std::min(statsIt->second.optimalLevel, maxAllowedDepth);
        }

        // Progressive deepening: try each level until confidence is high enough
        for (int level = 0; level <= effectiveDepth; level++) {
            ThinkResult levelResult = thinkAtLevel(level, query, profile, graph);

            // If this level gives good enough answer, stop
            if (levelResult.confidence >= HIGH_CONFIDENCE_THRESHOLD) {
                result = levelResult;
                result.depthUsed = level;
                break;
            }

            // If better than what we have so far, keep it
            if (levelResult.confidence > result.confidence) {
                result = levelResult;
                result.depthUsed = level;
            }

            // If we've hit medium confidence and next level is expensive, stop
            if (levelResult.confidence >= MEDIUM_CONFIDENCE_THRESHOLD &&
                level >= 3) {
                break;
            }
        }

        // Calculate elapsed time
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                            endTime - startTime);
        result.timeMs = (int)duration.count();

        // Record statistics for learning
        recordStats(profile.primaryConcept, result.depthUsed, result.confidence);

        return result;
    }

private:
    // ========================================================================
    // Private: Think at Specific Level
    // ========================================================================

    ThinkResult thinkAtLevel(int level, const std::string& query,
                             const QueryProfile& profile,
                             const std::vector<DepthGraphEdge>& graph) {
        ThinkResult result;
        result.confidence = 0.0f;
        result.depthUsed = level;
        result.fromCache = false;

        switch (level) {
            case 0:
                // Level 0: Heuristic bypass — instant pattern match
                result = heuristicLookup(query);
                break;

            case 1:
                // Level 1: Direct lookup — find exact fact
                result = directLookup(profile.primaryConcept, graph);
                break;

            case 2:
                // Level 2: Multi-hop — follow 2-3 edges
                result = multiHopLookup(profile.primaryConcept, graph, 3);
                break;

            case 3:
                // Level 3: Reasoning chains — apply inference rules
                result = reasoningChainLookup(query, profile, graph);
                break;

            case 4:
                // Level 4: Deep think — decompose and multi-query
                result = deepThink(query, profile, graph);
                break;

            case 5:
                // Level 5: Full analysis — hypothesis testing
                result = fullAnalysis(query, profile, graph);
                break;
        }

        return result;
    }

    // ========================================================================
    // Private: Level 0 — Heuristic Bypass
    // ========================================================================

    ThinkResult heuristicLookup(const std::string& query) {
        ThinkResult result;
        result.confidence = 0.0f;

        std::string queryLower = query;
        std::transform(queryLower.begin(), queryLower.end(),
                      queryLower.begin(), ::tolower);

        for (const auto& pair : heuristicPatterns) {
            if (queryLower.find(pair.first) != std::string::npos) {
                result.answer = pair.second;
                result.confidence = 1.0f;
                result.reasoning = "Heuristic pattern match: " + pair.first;
                return result;
            }
        }

        return result;
    }

    // ========================================================================
    // Private: Level 1 — Direct Lookup
    // ========================================================================

    ThinkResult directLookup(const std::string& concept,
                             const std::vector<DepthGraphEdge>& graph) {
        ThinkResult result;
        result.confidence = 0.0f;

        // Find any fact about this concept
        for (const auto& edge : graph) {
            if (edge.source == concept || edge.target == concept) {
                result.answer = edge.source + " " + edge.relation + " " + edge.target;
                result.confidence = edge.weight;
                result.reasoning = "Direct fact found";
                return result;
            }
        }

        return result;
    }

    // ========================================================================
    // Private: Level 2 — Multi-hop
    // ========================================================================

    ThinkResult multiHopLookup(const std::string& concept,
                                const std::vector<DepthGraphEdge>& graph,
                                int maxHops) {
        ThinkResult result;
        result.confidence = 0.0f;

        // BFS from concept, collecting facts along the way
        std::set<std::string> visited;
        std::vector<std::string> frontier = {concept};
        std::string path;

        for (int hop = 0; hop < maxHops && !frontier.empty(); hop++) {
            std::vector<std::string> nextFrontier;

            for (const auto& node : frontier) {
                if (visited.count(node) > 0) continue;
                visited.insert(node);

                for (const auto& edge : graph) {
                    if (edge.source == node && visited.count(edge.target) == 0) {
                        nextFrontier.push_back(edge.target);
                        if (!path.empty()) path += " → ";
                        path += edge.source + " " + edge.relation + " " + edge.target;
                    }
                }
            }

            frontier = nextFrontier;
        }

        if (!path.empty()) {
            result.answer = path;
            result.confidence = 0.6f;  // Multi-hop is less certain
            result.reasoning = "Multi-hop path (" + std::to_string(visited.size()) +
                             " nodes explored)";
        }

        return result;
    }


    // ========================================================================
    // Private: Level 3 — Reasoning Chains
    // ========================================================================

    ThinkResult reasoningChainLookup(const std::string& query,
                                      const QueryProfile& profile,
                                      const std::vector<DepthGraphEdge>& graph) {
        ThinkResult result;
        result.confidence = 0.0f;

        // Look for inference patterns:
        // If A is_a B, and B can X, then A can X (inheritance)
        std::string concept = profile.primaryConcept;

        // Find what this concept is_a
        std::vector<std::string> parents;
        for (const auto& edge : graph) {
            if (edge.source == concept &&
                (edge.relation == "is_a" || edge.relation == "type_of")) {
                parents.push_back(edge.target);
            }
        }

        // Find properties of parents that might apply
        std::string chain;
        for (const auto& parent : parents) {
            for (const auto& edge : graph) {
                if (edge.source == parent) {
                    if (!chain.empty()) chain += "; ";
                    chain += concept + " → is_a → " + parent +
                            " → " + edge.relation + " → " + edge.target;
                }
            }
        }

        if (!chain.empty()) {
            result.answer = chain;
            result.confidence = 0.7f;
            result.reasoning = "Inference via inheritance chain";
        }

        return result;
    }

    // ========================================================================
    // Private: Level 4 — Deep Think
    // ========================================================================

    ThinkResult deepThink(const std::string& query,
                           const QueryProfile& profile,
                           const std::vector<DepthGraphEdge>& graph) {
        ThinkResult result;
        result.confidence = 0.0f;

        // Decompose query into sub-questions
        // "Why can cats climb trees?" → "Can cats climb?" + "What enables climbing?"
        std::vector<std::string> subConcepts;
        std::istringstream stream(query);
        std::string word;
        std::set<std::string> stopwords = {
            "is", "a", "the", "of", "why", "how", "what", "can", "does", "do"
        };
        while (stream >> word) {
            std::string lower = word;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            if (stopwords.count(lower) == 0 && lower.size() > 2) {
                subConcepts.push_back(lower);
            }
        }

        // Query each sub-concept and combine results
        std::string combined;
        float totalConf = 0.0f;
        int found = 0;

        for (const auto& sub : subConcepts) {
            ThinkResult subResult = multiHopLookup(sub, graph, 2);
            if (subResult.confidence > 0.0f) {
                if (!combined.empty()) combined += " | ";
                combined += subResult.answer;
                totalConf += subResult.confidence;
                found++;
            }
        }

        if (found > 0) {
            result.answer = combined;
            result.confidence = totalConf / (float)found;
            result.reasoning = "Deep decomposition: " + std::to_string(found) +
                             " sub-queries resolved";
        }

        return result;
    }

    // ========================================================================
    // Private: Level 5 — Full Analysis
    // ========================================================================

    ThinkResult fullAnalysis(const std::string& query,
                              const QueryProfile& profile,
                              const std::vector<DepthGraphEdge>& graph) {
        ThinkResult result;
        result.confidence = 0.0f;

        // Full analysis: generate hypotheses and test them
        // 1. Deep think to get base understanding
        ThinkResult base = deepThink(query, profile, graph);

        // 2. Generate alternative interpretations
        std::vector<std::string> hypotheses;
        if (!profile.primaryConcept.empty()) {
            // Hypothesis 1: Direct answer
            hypotheses.push_back(profile.primaryConcept + " directly answers query");
            // Hypothesis 2: Related concept answers
            for (const auto& edge : graph) {
                if (edge.source == profile.primaryConcept) {
                    hypotheses.push_back(edge.target + " is relevant via " + edge.relation);
                }
            }
        }

        // 3. Test each hypothesis against graph
        std::string bestHypothesis;
        float bestScore = 0.0f;

        for (const auto& hyp : hypotheses) {
            // Score = how many supporting edges exist
            float score = 0.0f;
            for (const auto& edge : graph) {
                if (hyp.find(edge.source) != std::string::npos ||
                    hyp.find(edge.target) != std::string::npos) {
                    score += edge.weight;
                }
            }
            if (score > bestScore) {
                bestScore = score;
                bestHypothesis = hyp;
            }
        }

        // Combine base + hypothesis
        result.answer = base.answer;
        if (!bestHypothesis.empty()) {
            result.answer += " [Hypothesis: " + bestHypothesis + "]";
        }
        result.confidence = std::max(base.confidence, bestScore * 0.1f);
        result.reasoning = "Full analysis: " + std::to_string(hypotheses.size()) +
                         " hypotheses tested";

        return result;
    }

    // ========================================================================
    // Private: Statistics Recording
    // ========================================================================

    void recordStats(const std::string& queryType, int depthUsed, float confidence) {
        auto& stats = queryStats[queryType];
        stats.attempts++;
        if (depthUsed >= 0 && depthUsed <= 5) {
            stats.successAtLevel[depthUsed]++;
            // Running average confidence
            float prev = stats.avgConfidence[depthUsed];
            stats.avgConfidence[depthUsed] = prev * 0.9f + confidence * 0.1f;
        }

        // Update optimal level (the lowest level that typically gives good results)
        stats.optimalLevel = 0;
        float bestAvg = 0.0f;
        for (int i = 0; i <= 5; i++) {
            if (stats.avgConfidence[i] > bestAvg) {
                bestAvg = stats.avgConfidence[i];
                stats.optimalLevel = i;
            }
        }
    }

public:
    // ========================================================================
    // Utility
    // ========================================================================

    int getMaxDepth() const { return maxAllowedDepth; }
    int getThermalLevel() const { return thermalLevel; }

    // Get stats for a query type
    std::string getStatsReport() {
        std::string report = "[AdaptiveDepth Stats]\n";
        for (const auto& pair : queryStats) {
            report += "  " + pair.first + ": " +
                     std::to_string(pair.second.attempts) + " queries, " +
                     "optimal depth=" + std::to_string(pair.second.optimalLevel) + "\n";
        }
        return report;
    }
};

#endif // MK_ADAPTIVE_DEPTH_CPP
