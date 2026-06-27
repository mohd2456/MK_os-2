#ifndef MK_SELF_IMPROVER_CPP
#define MK_SELF_IMPROVER_CPP

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <ctime>
#include <cmath>

// ===================================================================================
// MK SELF-IMPROVEMENT ENGINE
// Tracks knowledge gaps, generates improvement plans, and measures growth over time.
// Features:
//   - Logs every query MK could not answer or answered with low confidence
//   - Categorizes knowledge gaps by topic
//   - Generates nightly "improvement plan" (topics to research)
//   - Tracks improvement over time (accuracy trending up/down)
//   - Integrates with autonomous_agent to auto-research gaps
//   - Produces weekly "growth reports" showing what was learned
// Designed for efficiency on low-end hardware (flat vectors, no heavy allocations).
// ===================================================================================

struct MKKnowledgeGap {
    std::string query;
    std::string topic;
    float confidence;       // The confidence that was too low
    std::time_t timestamp;
    bool researched;        // Has autonomous_agent looked this up?
    bool resolved;          // Has the gap been filled?
};

struct MKImprovementPlan {
    std::string date;
    std::vector<std::string> topicsToResearch;
    std::vector<std::string> gapQueries;    // Original queries that exposed gaps
    int priority;           // 1=critical, 2=important, 3=nice-to-have
    bool executed;
};

struct MKGrowthSnapshot {
    std::time_t timestamp;
    float overallAccuracy;
    int totalQueries;
    int answeredConfidently;
    int gaps;
    int gapsResolved;
};

struct MKWeeklyReport {
    std::string weekStart;
    std::string weekEnd;
    int queriesHandled;
    int newGapsFound;
    int gapsResolved;
    float accuracyStart;
    float accuracyEnd;
    float accuracyDelta;
    std::vector<std::string> topTopics;     // Most researched topics
    std::vector<std::string> improvements;  // What was learned
};

class MKSelfImprover {
private:
    std::vector<MKKnowledgeGap> gaps;
    std::vector<MKImprovementPlan> plans;
    std::vector<MKGrowthSnapshot> snapshots;
    std::unordered_map<std::string, int> topicGapCount;

    float confidenceThreshold;  // Below this = knowledge gap
    int totalQueries;
    int confidentAnswers;
    int lowConfidenceAnswers;
    int gapsResolved;

    std::string logFilePath;
    int maxGaps;

    // Categorize a query into a topic based on keywords
    std::string categorizeQuery(const std::string& query) {
        std::string lower = query;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        // Topic detection by keyword presence
        if (lower.find("python") != std::string::npos ||
            lower.find("javascript") != std::string::npos ||
            lower.find("code") != std::string::npos ||
            lower.find("programming") != std::string::npos ||
            lower.find("algorithm") != std::string::npos ||
            lower.find("function") != std::string::npos)
            return "programming";

        if (lower.find("physics") != std::string::npos ||
            lower.find("quantum") != std::string::npos ||
            lower.find("gravity") != std::string::npos ||
            lower.find("energy") != std::string::npos ||
            lower.find("force") != std::string::npos)
            return "physics";

        if (lower.find("history") != std::string::npos ||
            lower.find("war") != std::string::npos ||
            lower.find("ancient") != std::string::npos ||
            lower.find("century") != std::string::npos ||
            lower.find("empire") != std::string::npos)
            return "history";

        if (lower.find("math") != std::string::npos ||
            lower.find("calculus") != std::string::npos ||
            lower.find("equation") != std::string::npos ||
            lower.find("theorem") != std::string::npos ||
            lower.find("number") != std::string::npos)
            return "mathematics";

        if (lower.find("biology") != std::string::npos ||
            lower.find("cell") != std::string::npos ||
            lower.find("dna") != std::string::npos ||
            lower.find("evolution") != std::string::npos ||
            lower.find("species") != std::string::npos)
            return "biology";

        if (lower.find("chemistry") != std::string::npos ||
            lower.find("element") != std::string::npos ||
            lower.find("molecule") != std::string::npos ||
            lower.find("reaction") != std::string::npos)
            return "chemistry";

        if (lower.find("music") != std::string::npos ||
            lower.find("song") != std::string::npos ||
            lower.find("artist") != std::string::npos ||
            lower.find("album") != std::string::npos)
            return "music";

        if (lower.find("food") != std::string::npos ||
            lower.find("cook") != std::string::npos ||
            lower.find("recipe") != std::string::npos ||
            lower.find("nutrition") != std::string::npos)
            return "food_nutrition";

        if (lower.find("space") != std::string::npos ||
            lower.find("planet") != std::string::npos ||
            lower.find("star") != std::string::npos ||
            lower.find("nasa") != std::string::npos ||
            lower.find("orbit") != std::string::npos)
            return "space";

        if (lower.find("medicine") != std::string::npos ||
            lower.find("disease") != std::string::npos ||
            lower.find("treatment") != std::string::npos ||
            lower.find("symptom") != std::string::npos ||
            lower.find("health") != std::string::npos)
            return "medicine";

        if (lower.find("economy") != std::string::npos ||
            lower.find("market") != std::string::npos ||
            lower.find("inflation") != std::string::npos ||
            lower.find("stock") != std::string::npos ||
            lower.find("gdp") != std::string::npos)
            return "economics";

        if (lower.find("philosophy") != std::string::npos ||
            lower.find("ethics") != std::string::npos ||
            lower.find("logic") != std::string::npos ||
            lower.find("meaning") != std::string::npos)
            return "philosophy";

        if (lower.find("psychology") != std::string::npos ||
            lower.find("brain") != std::string::npos ||
            lower.find("behavior") != std::string::npos ||
            lower.find("cognitive") != std::string::npos ||
            lower.find("mental") != std::string::npos)
            return "psychology";

        return "general";
    }

    // Format timestamp for reports
    std::string formatDate(std::time_t t) {
        char buf[32];
        struct tm* tm_info = std::localtime(&t);
        if (tm_info) {
            std::strftime(buf, sizeof(buf), "%Y-%m-%d", tm_info);
            return std::string(buf);
        }
        return "unknown";
    }

    // Prune oldest gaps if over limit
    void pruneGaps() {
        if ((int)gaps.size() > maxGaps) {
            // Keep resolved gaps for a while, then remove oldest unresolved
            int toRemove = (int)gaps.size() - maxGaps;
            gaps.erase(gaps.begin(), gaps.begin() + toRemove);
        }
    }

public:
    MKSelfImprover(float threshold = 0.6f, int maxGapEntries = 10000,
                   const std::string& logPath = "mk_improvement.log")
        : confidenceThreshold(threshold), totalQueries(0),
          confidentAnswers(0), lowConfidenceAnswers(0), gapsResolved(0),
          logFilePath(logPath), maxGaps(maxGapEntries) {
        std::cout << "[IMPROVER] Self-improvement engine initialized.\n";
        std::cout << "[IMPROVER] Confidence threshold: " << confidenceThreshold
                  << " | Max gaps tracked: " << maxGaps << "\n";
    }

    // ─── Log Query Outcome ─────────────────────────────────────────────────────
    // Call this after every query is processed
    void logQueryOutcome(const std::string& query, float confidence, bool answered) {
        totalQueries++;

        if (answered && confidence >= confidenceThreshold) {
            confidentAnswers++;
        } else {
            lowConfidenceAnswers++;

            // Record as a knowledge gap
            MKKnowledgeGap gap;
            gap.query = query;
            gap.topic = categorizeQuery(query);
            gap.confidence = confidence;
            gap.timestamp = std::time(nullptr);
            gap.researched = false;
            gap.resolved = false;
            gaps.push_back(gap);

            topicGapCount[gap.topic]++;
            pruneGaps();

            std::cout << "[IMPROVER] Knowledge gap detected: \"" << query.substr(0, 60)
                      << "\" (topic: " << gap.topic
                      << ", confidence: " << (int)(confidence * 100) << "%)\n";
        }
    }

    // ─── Mark Gap as Researched ────────────────────────────────────────────────
    // Called by autonomous_agent when it starts researching a gap
    void markGapResearched(const std::string& query) {
        for (auto& gap : gaps) {
            if (gap.query == query && !gap.researched) {
                gap.researched = true;
                std::cout << "[IMPROVER] Gap marked as researched: \"" << query.substr(0, 40) << "\"\n";
                return;
            }
        }
    }

    // ─── Mark Gap as Resolved ──────────────────────────────────────────────────
    // Called when new knowledge is added that fills a gap
    void markGapResolved(const std::string& query) {
        for (auto& gap : gaps) {
            if (gap.query == query && !gap.resolved) {
                gap.resolved = true;
                gapsResolved++;
                std::cout << "[IMPROVER] Gap RESOLVED: \"" << query.substr(0, 40) << "\"\n";
                return;
            }
        }
    }

    // ─── Get Knowledge Gaps by Topic ───────────────────────────────────────────
    std::vector<MKKnowledgeGap> getGapsByTopic(const std::string& topic) {
        std::vector<MKKnowledgeGap> result;
        for (const auto& gap : gaps) {
            if (gap.topic == topic && !gap.resolved) {
                result.push_back(gap);
            }
        }
        return result;
    }

    // Get unresearched gaps (ready for autonomous_agent)
    std::vector<MKKnowledgeGap> getUnresearchedGaps(int maxCount = 20) {
        std::vector<MKKnowledgeGap> result;
        for (const auto& gap : gaps) {
            if (!gap.researched && !gap.resolved) {
                result.push_back(gap);
                if ((int)result.size() >= maxCount) break;
            }
        }
        return result;
    }

    // ─── Generate Nightly Improvement Plan ─────────────────────────────────────
    MKImprovementPlan generateImprovementPlan() {
        MKImprovementPlan plan;
        plan.date = formatDate(std::time(nullptr));
        plan.executed = false;

        // Find topics with the most gaps
        std::vector<std::pair<std::string, int>> sortedTopics(
            topicGapCount.begin(), topicGapCount.end());
        std::sort(sortedTopics.begin(), sortedTopics.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });

        // Top 5 topics to research
        int topicCount = std::min(5, (int)sortedTopics.size());
        for (int i = 0; i < topicCount; i++) {
            plan.topicsToResearch.push_back(sortedTopics[i].first);
        }

        // Collect recent unresolved gap queries (up to 10)
        int gapCount = 0;
        for (auto it = gaps.rbegin(); it != gaps.rend() && gapCount < 10; ++it) {
            if (!it->resolved) {
                plan.gapQueries.push_back(it->query);
                gapCount++;
            }
        }

        // Set priority based on accuracy
        float accuracy = getAccuracy();
        if (accuracy < 0.5f) plan.priority = 1;      // Critical
        else if (accuracy < 0.7f) plan.priority = 2;  // Important
        else plan.priority = 3;                        // Nice-to-have

        plans.push_back(plan);

        std::cout << "[IMPROVER] Generated improvement plan for " << plan.date
                  << " | Priority: " << plan.priority
                  << " | Topics: " << plan.topicsToResearch.size()
                  << " | Gaps to address: " << plan.gapQueries.size() << "\n";

        return plan;
    }

    // ─── Take Growth Snapshot ──────────────────────────────────────────────────
    void takeSnapshot() {
        MKGrowthSnapshot snap;
        snap.timestamp = std::time(nullptr);
        snap.overallAccuracy = getAccuracy();
        snap.totalQueries = totalQueries;
        snap.answeredConfidently = confidentAnswers;
        snap.gaps = (int)gaps.size();
        snap.gapsResolved = gapsResolved;
        snapshots.push_back(snap);

        std::cout << "[IMPROVER] Snapshot taken: accuracy=" << (int)(snap.overallAccuracy * 100)
                  << "%, gaps=" << snap.gaps << ", resolved=" << snap.gapsResolved << "\n";
    }

    // ─── Generate Weekly Growth Report ─────────────────────────────────────────
    MKWeeklyReport generateWeeklyReport() {
        MKWeeklyReport report;
        std::time_t now = std::time(nullptr);
        std::time_t weekAgo = now - (7 * 24 * 3600);

        report.weekEnd = formatDate(now);
        report.weekStart = formatDate(weekAgo);
        report.queriesHandled = totalQueries;
        report.accuracyEnd = getAccuracy();

        // Count gaps found and resolved this week
        report.newGapsFound = 0;
        report.gapsResolved = 0;
        for (const auto& gap : gaps) {
            if (gap.timestamp >= weekAgo) {
                report.newGapsFound++;
                if (gap.resolved) report.gapsResolved++;
            }
        }

        // Get accuracy from start of week (from snapshots)
        report.accuracyStart = report.accuracyEnd; // Default if no older snapshot
        for (const auto& snap : snapshots) {
            if (snap.timestamp <= weekAgo) {
                report.accuracyStart = snap.overallAccuracy;
            }
        }
        report.accuracyDelta = report.accuracyEnd - report.accuracyStart;

        // Top topics this week
        for (const auto& pair : topicGapCount) {
            if (pair.second > 2) {
                report.topTopics.push_back(pair.first + " (" + std::to_string(pair.second) + " gaps)");
            }
        }

        // Improvements (resolved gap topics)
        for (const auto& gap : gaps) {
            if (gap.resolved && gap.timestamp >= weekAgo) {
                report.improvements.push_back("Learned: " + gap.topic + " - " + gap.query.substr(0, 50));
            }
        }

        std::cout << "[IMPROVER] Weekly report generated: "
                  << report.weekStart << " to " << report.weekEnd << "\n";
        std::cout << "  Accuracy delta: " << (report.accuracyDelta >= 0 ? "+" : "")
                  << (int)(report.accuracyDelta * 100) << "%\n";
        std::cout << "  New gaps: " << report.newGapsFound
                  << " | Resolved: " << report.gapsResolved << "\n";

        return report;
    }

    // ─── Save Improvement Log to Disk ──────────────────────────────────────────
    bool saveLog() {
        std::ofstream file(logFilePath);
        if (!file.is_open()) return false;

        file << "# MK Self-Improvement Log\n";
        file << "# Generated: " << formatDate(std::time(nullptr)) << "\n\n";
        file << "## Statistics\n";
        file << "Total queries: " << totalQueries << "\n";
        file << "Confident answers: " << confidentAnswers << "\n";
        file << "Low confidence: " << lowConfidenceAnswers << "\n";
        file << "Gaps resolved: " << gapsResolved << "\n";
        file << "Current accuracy: " << (int)(getAccuracy() * 100) << "%\n\n";

        file << "## Knowledge Gaps by Topic\n";
        for (const auto& pair : topicGapCount) {
            file << "- " << pair.first << ": " << pair.second << " gaps\n";
        }
        file << "\n";

        file << "## Recent Unresolved Gaps\n";
        int count = 0;
        for (auto it = gaps.rbegin(); it != gaps.rend() && count < 50; ++it) {
            if (!it->resolved) {
                file << "- [" << formatDate(it->timestamp) << "] "
                     << it->topic << ": " << it->query << "\n";
                count++;
            }
        }

        file.close();
        std::cout << "[IMPROVER] Log saved to: " << logFilePath << "\n";
        return true;
    }

    // ─── Accuracy and Trend ────────────────────────────────────────────────────

    float getAccuracy() const {
        if (totalQueries == 0) return 0.0f;
        return (float)confidentAnswers / (float)totalQueries;
    }

    // Is accuracy trending up or down? (compares last 2 snapshots)
    std::string getAccuracyTrend() {
        if (snapshots.size() < 2) return "insufficient_data";
        float latest = snapshots.back().overallAccuracy;
        float previous = snapshots[snapshots.size() - 2].overallAccuracy;
        if (latest > previous + 0.02f) return "improving";
        if (latest < previous - 0.02f) return "declining";
        return "stable";
    }

    int getTotalGaps() const { return (int)gaps.size(); }
    int getResolvedGaps() const { return gapsResolved; }
    int getTotalQueries() const { return totalQueries; }

    void setConfidenceThreshold(float threshold) {
        confidenceThreshold = std::max(0.1f, std::min(0.95f, threshold));
    }

    void printStats() const {
        std::cout << "[IMPROVER STATS] Queries: " << totalQueries
                  << " | Confident: " << confidentAnswers
                  << " | Low confidence: " << lowConfidenceAnswers
                  << " | Accuracy: " << (int)(getAccuracy() * 100) << "%\n";
        std::cout << "  Total gaps: " << gaps.size()
                  << " | Resolved: " << gapsResolved
                  << " | Topics tracked: " << topicGapCount.size() << "\n";
    }
};

#endif // MK_SELF_IMPROVER_CPP
