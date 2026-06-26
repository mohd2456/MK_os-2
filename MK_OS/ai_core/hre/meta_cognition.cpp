#ifndef MK_META_COGNITION_CPP
#define MK_META_COGNITION_CPP

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <ctime>
#include <algorithm>
#include <numeric>
#include <deque>

// ===================================================================================
// MK META-COGNITION ENGINE — Self-awareness & Self-improvement
// ===================================================================================
// MK knows what it DOESN'T know, and actively works to fix that.
// This is the engine that makes MK get smarter over time without training.
//
// Features:
//   - Tracks uncertainty: logs every question it couldn't answer well
//   - Knowledge gap detection: identifies topics where it's weak
//   - Response quality scoring: rates its own answers
//   - Repetition avoidance: detects when it's saying the same things
//   - Rule effectiveness: tracks which reasoning rules produce good results
//   - Resource awareness: adjusts complexity based on available CPU/RAM
//   - Self-reflection log: stores insights about its own performance
//
// This is what separates MK from a static lookup tool.
// MK KNOWS it's limited and actively seeks to grow.
// ===================================================================================


struct MKUncertaintyEntry {
    std::string query;
    std::string bestAttempt;
    float confidence;
    std::time_t timestamp;
    bool resolved;          // Has this been answered since?
    std::string resolution; // The eventual answer (once learned)
};

struct MKKnowledgeGap {
    std::string topic;
    int missCount;          // How many times MK failed on this topic
    std::string lastQuery;  // Most recent failed query about this
    bool prioritized;       // Queued for active learning
};

struct MKResponseQuality {
    std::string query;
    std::string response;
    float selfScore;        // MK's own assessment (0-1)
    float userFeedback;     // User's rating if given (-1 = none, 0-1 = rated)
    std::time_t timestamp;
};

struct MKRulePerformance {
    std::string ruleName;
    int timesUsed;
    int timesHelpful;       // Led to a good answer
    int timesMisleading;    // Led to wrong answer
    float effectivenessScore;
};

struct MKSelfInsight {
    std::time_t timestamp;
    std::string category;   // "strength", "weakness", "improvement", "pattern"
    std::string insight;
};

class MKMetaCognition {
private:
    // Uncertainty tracking
    std::vector<MKUncertaintyEntry> uncertaintyLog;
    std::map<std::string, MKKnowledgeGap> knowledgeGaps;
    
    // Quality tracking
    std::deque<MKResponseQuality> recentResponses;
    int maxRecentResponses;
    
    // Rule effectiveness
    std::map<std::string, MKRulePerformance> ruleStats;
    
    // Self-insights
    std::vector<MKSelfInsight> insights;
    
    // Repetition detection
    std::deque<std::string> recentOutputs;
    int maxOutputMemory;
    
    // Resource awareness
    float currentCPUUsage;
    float currentRAMUsage;
    float complexityBudget;     // 0.0 to 1.0, how complex we can be right now
    
    // Stats
    int totalInteractions;
    int successfulAnswers;
    int failedAnswers;
    int learningEvents;
    std::string logFilePath;

    // Calculate similarity between two strings (for repetition detection)
    float stringSimilarity(const std::string& a, const std::string& b) {
        if (a.empty() || b.empty()) return 0.0f;
        int matches = 0;
        std::string shorter = (a.size() < b.size()) ? a : b;
        std::string longer = (a.size() >= b.size()) ? a : b;
        for (char c : shorter) {
            if (longer.find(c) != std::string::npos) matches++;
        }
        return (float)matches / longer.size();
    }

public:
    MKMetaCognition(const std::string& logFile = "mk_meta_log.txt") 
        : maxRecentResponses(100), maxOutputMemory(50), currentCPUUsage(0.0f),
          currentRAMUsage(0.0f), complexityBudget(1.0f), totalInteractions(0),
          successfulAnswers(0), failedAnswers(0), learningEvents(0), logFilePath(logFile) {
        std::cout << "[META] Self-awareness engine initialized.\n";
    }

    // ─────────────────────────────────────────
    //  UNCERTAINTY TRACKING
    // ─────────────────────────────────────────

    // Log a query MK couldn't answer well
    void logUncertainty(const std::string& query, const std::string& attempt, float confidence) {
        MKUncertaintyEntry entry;
        entry.query = query;
        entry.bestAttempt = attempt;
        entry.confidence = confidence;
        entry.timestamp = std::time(nullptr);
        entry.resolved = false;
        uncertaintyLog.push_back(entry);
        failedAnswers++;
        
        // Track as knowledge gap
        std::string lower = query;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        
        // Extract main topic (largest word)
        std::string topic;
        std::stringstream ss(lower);
        std::string word;
        while (ss >> word) {
            if (word.size() > topic.size() && word.size() > 3) topic = word;
        }
        
        if (!topic.empty()) {
            knowledgeGaps[topic].topic = topic;
            knowledgeGaps[topic].missCount++;
            knowledgeGaps[topic].lastQuery = query;
        }
    }

    // Mark an uncertainty as resolved (MK learned the answer)
    void resolveUncertainty(const std::string& query, const std::string& answer) {
        for (auto& entry : uncertaintyLog) {
            if (entry.query == query && !entry.resolved) {
                entry.resolved = true;
                entry.resolution = answer;
                learningEvents++;
                break;
            }
        }
    }

    // Get topics MK should learn about (sorted by frequency of failure)
    std::vector<MKKnowledgeGap> getTopGaps(int maxCount = 10) {
        std::vector<MKKnowledgeGap> gaps;
        for (const auto& kv : knowledgeGaps) {
            if (!kv.second.prioritized) {
                gaps.push_back(kv.second);
            }
        }
        std::sort(gaps.begin(), gaps.end(), 
                  [](const MKKnowledgeGap& a, const MKKnowledgeGap& b) {
                      return a.missCount > b.missCount;
                  });
        if ((int)gaps.size() > maxCount) gaps.resize(maxCount);
        return gaps;
    }

    // ─────────────────────────────────────────
    //  RESPONSE QUALITY SCORING
    // ─────────────────────────────────────────

    // Score MK's own response quality
    float scoreResponse(const std::string& query, const std::string& response,
                        float confidence, int factsUsed, int reasoningSteps) {
        totalInteractions++;
        float score = 0.0f;
        
        // Factor 1: Confidence from the reasoning engine
        score += confidence * 0.3f;
        
        // Factor 2: Did we use actual facts? (vs just templates)
        score += std::min(1.0f, factsUsed * 0.15f) * 0.25f;
        
        // Factor 3: Response length (too short = bad, too long = bad)
        float lenScore = 0.0f;
        if (response.size() > 20 && response.size() < 500) lenScore = 1.0f;
        else if (response.size() > 10) lenScore = 0.5f;
        score += lenScore * 0.15f;
        
        // Factor 4: Reasoning depth
        score += std::min(1.0f, reasoningSteps * 0.2f) * 0.2f;
        
        // Factor 5: Not repetitive (compare to recent outputs)
        float novelty = 1.0f;
        for (const auto& prev : recentOutputs) {
            float sim = stringSimilarity(response, prev);
            if (sim > 0.8f) { novelty = 0.3f; break; }
        }
        score += novelty * 0.1f;
        
        // Track
        MKResponseQuality qual;
        qual.query = query;
        qual.response = response;
        qual.selfScore = score;
        qual.userFeedback = -1.0f;  // Not yet rated
        qual.timestamp = std::time(nullptr);
        recentResponses.push_back(qual);
        if ((int)recentResponses.size() > maxRecentResponses) recentResponses.pop_front();
        
        // Track output for repetition detection
        recentOutputs.push_back(response);
        if ((int)recentOutputs.size() > maxOutputMemory) recentOutputs.pop_front();
        
        if (score > 0.5f) successfulAnswers++;
        else logUncertainty(query, response, confidence);
        
        return score;
    }

    // User gives feedback on a response
    void userFeedback(const std::string& query, float rating) {
        for (auto it = recentResponses.rbegin(); it != recentResponses.rend(); ++it) {
            if (it->query == query) {
                it->userFeedback = rating;
                break;
            }
        }
    }

    // ─────────────────────────────────────────
    //  SELF-REFLECTION
    // ─────────────────────────────────────────

    // Generate a self-reflection based on recent performance
    std::string reflect() {
        std::stringstream reflection;
        
        float avgScore = 0.0f;
        int scored = 0;
        for (const auto& r : recentResponses) {
            avgScore += r.selfScore;
            scored++;
        }
        if (scored > 0) avgScore /= scored;
        
        reflection << "[SELF-REFLECTION]\n";
        reflection << "Interactions: " << totalInteractions << "\n";
        reflection << "Success rate: " << (totalInteractions > 0 ? 
            (successfulAnswers * 100 / totalInteractions) : 0) << "%\n";
        reflection << "Average quality: " << (avgScore * 100) << "%\n";
        reflection << "Knowledge gaps: " << knowledgeGaps.size() << " topics\n";
        reflection << "Unresolved questions: " << 
            std::count_if(uncertaintyLog.begin(), uncertaintyLog.end(),
                         [](const MKUncertaintyEntry& e) { return !e.resolved; }) << "\n";
        
        // Generate insight
        if (avgScore < 0.4f) {
            reflection << "INSIGHT: My answers are weak. I need more knowledge loaded.\n";
            addInsight("weakness", "Low average response quality — need more facts in graph.");
        } else if (avgScore > 0.7f) {
            reflection << "INSIGHT: Performing well on recent queries.\n";
            addInsight("strength", "High quality answers — current knowledge base is effective.");
        }
        
        auto gaps = getTopGaps(3);
        if (!gaps.empty()) {
            reflection << "TOP GAPS: ";
            for (const auto& g : gaps) reflection << g.topic << "(" << g.missCount << " misses) ";
            reflection << "\n";
        }
        
        return reflection.str();
    }

    void addInsight(const std::string& category, const std::string& insight) {
        MKSelfInsight si;
        si.timestamp = std::time(nullptr);
        si.category = category;
        si.insight = insight;
        insights.push_back(si);
    }

    // ─────────────────────────────────────────
    //  RESOURCE AWARENESS
    // ─────────────────────────────────────────

    void updateResources(float cpuPercent, float ramPercent) {
        currentCPUUsage = cpuPercent;
        currentRAMUsage = ramPercent;
        
        // Adjust complexity budget based on available resources
        if (ramPercent > 80.0f || cpuPercent > 85.0f) {
            complexityBudget = 0.3f;  // Low resources — keep it simple
        } else if (ramPercent > 60.0f || cpuPercent > 60.0f) {
            complexityBudget = 0.6f;  // Medium — moderate reasoning
        } else {
            complexityBudget = 1.0f;  // Full power
        }
    }

    float getComplexityBudget() const { return complexityBudget; }
    
    // Should MK do deep reasoning or stay shallow?
    int getRecommendedDepth() {
        if (complexityBudget >= 0.8f) return 10;  // Full depth
        if (complexityBudget >= 0.5f) return 5;   // Medium
        return 2;                                   // Quick answers only
    }

    // ─────────────────────────────────────────
    //  PERSISTENCE
    // ─────────────────────────────────────────

    void saveState() {
        std::ofstream out(logFilePath);
        if (!out.is_open()) return;
        
        out << "# MK Meta-Cognition State\n";
        out << "interactions|" << totalInteractions << "\n";
        out << "successes|" << successfulAnswers << "\n";
        out << "failures|" << failedAnswers << "\n";
        out << "learning_events|" << learningEvents << "\n";
        out << "# Knowledge Gaps\n";
        for (const auto& kv : knowledgeGaps) {
            out << "gap|" << kv.second.topic << "|" << kv.second.missCount << "\n";
        }
        out.close();
    }

    void printStats() const {
        std::cout << "\n--- [META-COGNITION] ---\n";
        std::cout << "  Interactions: " << totalInteractions << "\n";
        std::cout << "  Successes: " << successfulAnswers << " | Failures: " << failedAnswers << "\n";
        std::cout << "  Knowledge gaps: " << knowledgeGaps.size() << "\n";
        std::cout << "  Insights logged: " << insights.size() << "\n";
        std::cout << "  Complexity budget: " << (complexityBudget * 100) << "%\n";
        std::cout << "------------------------\n";
    }
};

#endif // MK_META_COGNITION_CPP
