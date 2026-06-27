#ifndef MK_DEBATE_ENGINE_CPP
#define MK_DEBATE_ENGINE_CPP

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <algorithm>
#include <numeric>
#include <functional>
#include <sstream>

// ===================================================================================
// MK DEBATE ENGINE - CONFLICT RESOLUTION THROUGH STRUCTURED ARGUMENTATION
// When MK encounters conflicting information from different sources, it does not
// simply discard one or guess. Instead it:
//   - Presents BOTH sides with evidence for each
//   - Scores each side by: number of sources, source trust level, recency, consistency
//   - Declares a "winner" with reasoning, OR admits "genuinely contested"
//   - Stores the debate result so analysis is not repeated
//   - Allows the user to settle the debate (user word = final)
// ===================================================================================

enum class DebateVerdict {
    SIDE_A_WINS,            // Side A has stronger evidence
    SIDE_B_WINS,            // Side B has stronger evidence
    GENUINELY_CONTESTED,    // Both sides have merit, no clear winner
    USER_DECIDED,           // The user settled the debate
    PENDING                 // Debate not yet resolved
};

struct DebateEvidence {
    std::string sourceUrl;
    std::string snippet;
    float trustLevel;       // 0.0 to 1.0 based on source reputation
    std::time_t publishedAt;
    bool logicallyConsistent;

    float computeScore() const {
        float score = trustLevel;
        // Recency bonus: content less than 30 days old gets a boost
        std::time_t now = std::time(nullptr);
        double ageDays = std::difftime(now, publishedAt) / 86400.0;
        if (ageDays < 30) score += 0.1f;
        if (ageDays < 7) score += 0.1f;
        if (logicallyConsistent) score += 0.15f;
        return std::min(score, 1.0f);
    }
};

struct DebateSide {
    std::string claim;
    std::string label;       // e.g., "Side A" or a short descriptor
    std::vector<DebateEvidence> evidence;

    int sourceCount() const {
        return static_cast<int>(evidence.size());
    }

    float averageTrust() const {
        if (evidence.empty()) return 0.0f;
        float sum = 0.0f;
        for (const auto& e : evidence) sum += e.trustLevel;
        return sum / static_cast<float>(evidence.size());
    }

    float totalScore() const {
        if (evidence.empty()) return 0.0f;
        float sum = 0.0f;
        for (const auto& e : evidence) sum += e.computeScore();
        return sum;
    }

    float recencyScore() const {
        if (evidence.empty()) return 0.0f;
        std::time_t now = std::time(nullptr);
        float total = 0.0f;
        for (const auto& e : evidence) {
            double ageDays = std::difftime(now, e.publishedAt) / 86400.0;
            if (ageDays < 7) total += 1.0f;
            else if (ageDays < 30) total += 0.7f;
            else if (ageDays < 90) total += 0.4f;
            else total += 0.2f;
        }
        return total / static_cast<float>(evidence.size());
    }

    int logicalConsistencyCount() const {
        int count = 0;
        for (const auto& e : evidence) {
            if (e.logicallyConsistent) count++;
        }
        return count;
    }
};

struct DebateResult {
    std::string topic;
    DebateSide sideA;
    DebateSide sideB;
    DebateVerdict verdict;
    std::string reasoning;
    std::time_t resolvedAt;
    std::string userDecision;   // If user settled it
    float confidenceInVerdict;  // How confident MK is in the verdict

    std::string verdictToString() const {
        switch (verdict) {
            case DebateVerdict::SIDE_A_WINS: return "SIDE_A_WINS";
            case DebateVerdict::SIDE_B_WINS: return "SIDE_B_WINS";
            case DebateVerdict::GENUINELY_CONTESTED: return "GENUINELY_CONTESTED";
            case DebateVerdict::USER_DECIDED: return "USER_DECIDED";
            case DebateVerdict::PENDING: return "PENDING";
        }
        return "UNKNOWN";
    }
};

struct DebateScoreBreakdown {
    float sourceCountScore;
    float trustScore;
    float recencyScore;
    float consistencyScore;
    float totalWeighted;
};

class MKDebateEngine {
private:
    std::vector<DebateResult> debateHistory;
    std::map<std::string, DebateResult> resolvedDebates;  // topic -> result cache
    int totalDebates;
    int sideAWins;
    int sideBWins;
    int contested;
    int userDecided;

    // Scoring weights
    float weightSourceCount;
    float weightTrust;
    float weightRecency;
    float weightConsistency;

    // Threshold: if difference is below this, it is genuinely contested
    float contestedThreshold;

public:
    MKDebateEngine()
        : totalDebates(0), sideAWins(0), sideBWins(0), contested(0), userDecided(0),
          weightSourceCount(0.25f), weightTrust(0.35f), weightRecency(0.20f),
          weightConsistency(0.20f), contestedThreshold(0.15f) {
        std::cout << "[DEBATE_ENGINE] Initialized - conflict resolution through structured argumentation\n";
        std::cout << "[DEBATE_ENGINE] Weights: sources=" << weightSourceCount
                  << " trust=" << weightTrust << " recency=" << weightRecency
                  << " consistency=" << weightConsistency << "\n";
    }

    // Check if a topic has already been debated and resolved
    bool hasResolvedDebate(const std::string& topic) const {
        return resolvedDebates.find(topic) != resolvedDebates.end();
    }

    // Retrieve a previously resolved debate
    DebateResult getResolvedDebate(const std::string& topic) const {
        auto it = resolvedDebates.find(topic);
        if (it != resolvedDebates.end()) {
            std::cout << "[DEBATE_ENGINE] Cache hit for topic: " << topic << "\n";
            return it->second;
        }
        // Return empty pending result
        DebateResult empty;
        empty.topic = topic;
        empty.verdict = DebateVerdict::PENDING;
        return empty;
    }

    // Score a single side with detailed breakdown
    DebateScoreBreakdown scoreSide(const DebateSide& side) const {
        DebateScoreBreakdown breakdown;

        // Source count score (normalized: 5+ sources = max)
        float rawSourceScore = static_cast<float>(side.sourceCount()) / 5.0f;
        breakdown.sourceCountScore = std::min(rawSourceScore, 1.0f);

        // Trust score (average trust level)
        breakdown.trustScore = side.averageTrust();

        // Recency score
        breakdown.recencyScore = side.recencyScore();

        // Logical consistency score
        float consistentRatio = side.evidence.empty() ? 0.0f :
            static_cast<float>(side.logicalConsistencyCount()) / static_cast<float>(side.evidence.size());
        breakdown.consistencyScore = consistentRatio;

        // Weighted total
        breakdown.totalWeighted =
            (breakdown.sourceCountScore * weightSourceCount) +
            (breakdown.trustScore * weightTrust) +
            (breakdown.recencyScore * weightRecency) +
            (breakdown.consistencyScore * weightConsistency);

        return breakdown;
    }

    // Run a full debate between two conflicting claims
    DebateResult conductDebate(const std::string& topic, DebateSide sideA, DebateSide sideB) {
        std::cout << "[DEBATE_ENGINE] Conducting debate on: " << topic << "\n";
        std::cout << "[DEBATE_ENGINE] Side A (\"" << sideA.label << "\"): "
                  << sideA.sourceCount() << " sources\n";
        std::cout << "[DEBATE_ENGINE] Side B (\"" << sideB.label << "\"): "
                  << sideB.sourceCount() << " sources\n";

        // Check cache first
        if (hasResolvedDebate(topic)) {
            std::cout << "[DEBATE_ENGINE] Already resolved this topic, returning cached result\n";
            return getResolvedDebate(topic);
        }

        // Score both sides
        DebateScoreBreakdown scoreA = scoreSide(sideA);
        DebateScoreBreakdown scoreB = scoreSide(sideB);

        std::cout << "[DEBATE_ENGINE] Score A: " << scoreA.totalWeighted
                  << " (sources=" << scoreA.sourceCountScore
                  << " trust=" << scoreA.trustScore
                  << " recency=" << scoreA.recencyScore
                  << " consistency=" << scoreA.consistencyScore << ")\n";
        std::cout << "[DEBATE_ENGINE] Score B: " << scoreB.totalWeighted
                  << " (sources=" << scoreB.sourceCountScore
                  << " trust=" << scoreB.trustScore
                  << " recency=" << scoreB.recencyScore
                  << " consistency=" << scoreB.consistencyScore << ")\n";

        // Determine verdict
        DebateResult result;
        result.topic = topic;
        result.sideA = sideA;
        result.sideB = sideB;
        result.resolvedAt = std::time(nullptr);

        float diff = scoreA.totalWeighted - scoreB.totalWeighted;

        if (std::abs(diff) < contestedThreshold) {
            result.verdict = DebateVerdict::GENUINELY_CONTESTED;
            result.reasoning = "Both sides have comparable evidence strength. "
                             "Side A scored " + std::to_string(scoreA.totalWeighted) +
                             " vs Side B scored " + std::to_string(scoreB.totalWeighted) +
                             ". The difference (" + std::to_string(std::abs(diff)) +
                             ") is below the contested threshold (" +
                             std::to_string(contestedThreshold) + ").";
            result.confidenceInVerdict = 0.5f;
            contested++;
            std::cout << "[DEBATE_ENGINE] Verdict: GENUINELY CONTESTED\n";
        } else if (diff > 0) {
            result.verdict = DebateVerdict::SIDE_A_WINS;
            result.reasoning = "Side A (\"" + sideA.label + "\") wins with score " +
                             std::to_string(scoreA.totalWeighted) + " vs " +
                             std::to_string(scoreB.totalWeighted) + ". ";
            result.reasoning += buildWinReason(scoreA, scoreB, "A");
            result.confidenceInVerdict = std::min(0.5f + diff, 1.0f);
            sideAWins++;
            std::cout << "[DEBATE_ENGINE] Verdict: SIDE A WINS\n";
        } else {
            result.verdict = DebateVerdict::SIDE_B_WINS;
            result.reasoning = "Side B (\"" + sideB.label + "\") wins with score " +
                             std::to_string(scoreB.totalWeighted) + " vs " +
                             std::to_string(scoreA.totalWeighted) + ". ";
            result.reasoning += buildWinReason(scoreB, scoreA, "B");
            result.confidenceInVerdict = std::min(0.5f + std::abs(diff), 1.0f);
            sideBWins++;
            std::cout << "[DEBATE_ENGINE] Verdict: SIDE B WINS\n";
        }

        // Store result
        totalDebates++;
        debateHistory.push_back(result);
        resolvedDebates[topic] = result;

        return result;
    }

    // Allow the user to settle a debate - their word is final
    DebateResult userSettlesDebate(const std::string& topic, const std::string& userDecisionText,
                                   bool userPicksSideA) {
        std::cout << "[DEBATE_ENGINE] User settling debate on: " << topic << "\n";
        std::cout << "[DEBATE_ENGINE] User decision: " << userDecisionText << "\n";

        DebateResult result;
        if (hasResolvedDebate(topic)) {
            result = resolvedDebates[topic];
        } else {
            result.topic = topic;
        }

        result.verdict = DebateVerdict::USER_DECIDED;
        result.userDecision = userDecisionText;
        result.confidenceInVerdict = 1.0f;  // User word is absolute
        result.resolvedAt = std::time(nullptr);
        result.reasoning = "User settled this debate. Their decision: " + userDecisionText;

        if (userPicksSideA) {
            result.reasoning += " (User agrees with Side A: \"" + result.sideA.label + "\")";
        } else {
            result.reasoning += " (User agrees with Side B: \"" + result.sideB.label + "\")";
        }

        resolvedDebates[topic] = result;
        userDecided++;

        std::cout << "[DEBATE_ENGINE] Debate settled by user authority. Stored permanently.\n";
        return result;
    }

    // Present both sides in a readable format
    std::string presentBothSides(const DebateResult& result) const {
        std::string output;
        output += "=== DEBATE: " + result.topic + " ===\n\n";

        output += "--- SIDE A: " + result.sideA.label + " ---\n";
        output += "Claim: " + result.sideA.claim + "\n";
        output += "Sources: " + std::to_string(result.sideA.sourceCount()) + "\n";
        output += "Average trust: " + std::to_string(result.sideA.averageTrust()) + "\n";
        for (const auto& e : result.sideA.evidence) {
            output += "  - [" + std::to_string(e.trustLevel) + "] " + e.snippet;
            output += " (" + e.sourceUrl + ")\n";
        }

        output += "\n--- SIDE B: " + result.sideB.label + " ---\n";
        output += "Claim: " + result.sideB.claim + "\n";
        output += "Sources: " + std::to_string(result.sideB.sourceCount()) + "\n";
        output += "Average trust: " + std::to_string(result.sideB.averageTrust()) + "\n";
        for (const auto& e : result.sideB.evidence) {
            output += "  - [" + std::to_string(e.trustLevel) + "] " + e.snippet;
            output += " (" + e.sourceUrl + ")\n";
        }

        output += "\n--- VERDICT ---\n";
        output += result.verdictToString() + "\n";
        output += "Reasoning: " + result.reasoning + "\n";
        output += "Confidence: " + std::to_string(result.confidenceInVerdict) + "\n";

        return output;
    }

    // Ask user to settle - returns the prompt text
    std::string askUserToSettle(const std::string& topic) const {
        std::string prompt;
        prompt += "[DEBATE_ENGINE] I found conflicting information about: " + topic + "\n";

        if (hasResolvedDebate(topic)) {
            auto it = resolvedDebates.find(topic);
            if (it != resolvedDebates.end()) {
                const DebateResult& r = it->second;
                prompt += "Side A says: " + r.sideA.claim + "\n";
                prompt += "Side B says: " + r.sideB.claim + "\n";
                prompt += "\nWhich do you believe is correct? (Your word is final)\n";
                prompt += "Options:\n";
                prompt += "  1) Side A is correct\n";
                prompt += "  2) Side B is correct\n";
                prompt += "  3) Neither / it depends on context\n";
            }
        } else {
            prompt += "I have not analyzed this topic yet. Would you like me to research it?\n";
        }

        return prompt;
    }

    // Configure scoring weights
    void setWeights(float wSource, float wTrust, float wRecency, float wConsistency) {
        weightSourceCount = wSource;
        weightTrust = wTrust;
        weightRecency = wRecency;
        weightConsistency = wConsistency;
        std::cout << "[DEBATE_ENGINE] Updated weights: sources=" << wSource
                  << " trust=" << wTrust << " recency=" << wRecency
                  << " consistency=" << wConsistency << "\n";
    }

    // Set the threshold for declaring something "genuinely contested"
    void setContestedThreshold(float threshold) {
        contestedThreshold = threshold;
        std::cout << "[DEBATE_ENGINE] Contested threshold set to: " << threshold << "\n";
    }

    // Get debate history for a topic
    std::vector<DebateResult> getDebatesForTopic(const std::string& keyword) const {
        std::vector<DebateResult> matches;
        for (const auto& d : debateHistory) {
            if (d.topic.find(keyword) != std::string::npos) {
                matches.push_back(d);
            }
        }
        return matches;
    }

    // Clear the cache for a specific topic (force re-analysis)
    void invalidateDebate(const std::string& topic) {
        auto it = resolvedDebates.find(topic);
        if (it != resolvedDebates.end()) {
            resolvedDebates.erase(it);
            std::cout << "[DEBATE_ENGINE] Invalidated cached debate for: " << topic << "\n";
        }
    }

    // Print engine stats
    void printStats() const {
        std::cout << "\n=== DEBATE ENGINE STATS ===\n";
        std::cout << "Total debates conducted: " << totalDebates << "\n";
        std::cout << "Side A wins: " << sideAWins << "\n";
        std::cout << "Side B wins: " << sideBWins << "\n";
        std::cout << "Genuinely contested: " << contested << "\n";
        std::cout << "User-decided: " << userDecided << "\n";
        std::cout << "Cached resolutions: " << resolvedDebates.size() << "\n";
        std::cout << "Scoring weights: sources=" << weightSourceCount
                  << " trust=" << weightTrust << " recency=" << weightRecency
                  << " consistency=" << weightConsistency << "\n";
        std::cout << "Contested threshold: " << contestedThreshold << "\n";
        std::cout << "===========================\n\n";
    }

private:
    // Build a human-readable explanation of why a side won
    std::string buildWinReason(const DebateScoreBreakdown& winner,
                               const DebateScoreBreakdown& loser,
                               const std::string& winnerLabel) const {
        std::string reason = "Key factors: ";
        std::vector<std::string> factors;

        if (winner.sourceCountScore > loser.sourceCountScore) {
            factors.push_back("more supporting sources");
        }
        if (winner.trustScore > loser.trustScore) {
            factors.push_back("higher source trustworthiness");
        }
        if (winner.recencyScore > loser.recencyScore) {
            factors.push_back("more recent evidence");
        }
        if (winner.consistencyScore > loser.consistencyScore) {
            factors.push_back("better logical consistency");
        }

        if (factors.empty()) {
            reason += "marginal overall advantage across all metrics";
        } else {
            for (size_t i = 0; i < factors.size(); i++) {
                if (i > 0) reason += ", ";
                reason += factors[i];
            }
        }

        return reason;
    }
};

#endif // MK_DEBATE_ENGINE_CPP
