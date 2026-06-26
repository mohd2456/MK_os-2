#ifndef MK_CONFIDENCE_CPP
#define MK_CONFIDENCE_CPP

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <numeric>

// ===================================================================================
// MK CONFIDENCE SCORING & UNCERTAINTY ESTIMATION
// Evaluates the quality and reliability of MK's outputs to decide:
//   - Can this be answered by the tiny student model? (FAST_TRACK)
//   - Should it escalate to the full cascade? (CASCADE_THINK)
//   - Should it be flagged as uncertain? (TO_LEARN list)
//   - Should it be offloaded to a peer node? (MESH_OFFLOAD)
// Uses entropy analysis of output distributions + heuristic signals.
// ===================================================================================

enum class MKConfidenceLevel {
    CERTAIN,        // >90% — answer directly
    CONFIDENT,      // 70-90% — answer but note uncertainty
    UNCERTAIN,      // 40-70% — escalate to deeper model
    VERY_UNCERTAIN, // 20-40% — try mesh offload or mark as "to learn"
    UNKNOWN         // <20% — cannot answer, save to learning queue
};

enum class MKEscalationPath {
    ANSWER_DIRECTLY,    // Student model can handle it
    ESCALATE_CASCADE,   // Need deeper reasoning
    ESCALATE_RAG,       // Need to retrieve knowledge first
    ESCALATE_MESH,      // Offload to peer device
    DEFER_TO_USER,      // Ask user for clarification
    MARK_UNKNOWN        // Log as unknown and skip
};

struct MKConfidenceResult {
    float score;                  // 0.0 to 1.0
    MKConfidenceLevel level;
    MKEscalationPath path;
    float entropy;                // Shannon entropy of output distribution
    std::string reasoning;        // Why this confidence was assigned
};

class MKConfidenceScorer {
private:
    // Thresholds (tunable per hardware capability)
    float certainThreshold;
    float confidentThreshold;
    float uncertainThreshold;
    float veryUncertainThreshold;
    
    // History for calibration
    std::vector<float> recentScores;
    int maxHistory;
    int totalEvaluations;
    int escalations;

    // Calculate Shannon entropy from a probability distribution
    float calculateEntropy(const std::vector<float>& probs) {
        float entropy = 0.0f;
        for (float p : probs) {
            if (p > 1e-10f) {
                entropy -= p * std::log2(p);
            }
        }
        return entropy;
    }
    
    // Normalize raw logits to probabilities via softmax
    std::vector<float> softmax(const std::vector<float>& logits) {
        std::vector<float> probs(logits.size());
        float maxVal = *std::max_element(logits.begin(), logits.end());
        float sum = 0.0f;
        for (size_t i = 0; i < logits.size(); i++) {
            probs[i] = std::exp(logits[i] - maxVal);
            sum += probs[i];
        }
        for (auto& p : probs) p /= sum;
        return probs;
    }

    MKConfidenceLevel scoreToLevel(float score) {
        if (score >= certainThreshold) return MKConfidenceLevel::CERTAIN;
        if (score >= confidentThreshold) return MKConfidenceLevel::CONFIDENT;
        if (score >= uncertainThreshold) return MKConfidenceLevel::UNCERTAIN;
        if (score >= veryUncertainThreshold) return MKConfidenceLevel::VERY_UNCERTAIN;
        return MKConfidenceLevel::UNKNOWN;
    }
    
    MKEscalationPath determineEscalation(MKConfidenceLevel level, bool ragAvailable, 
                                          bool meshAvailable) {
        switch (level) {
            case MKConfidenceLevel::CERTAIN:
            case MKConfidenceLevel::CONFIDENT:
                return MKEscalationPath::ANSWER_DIRECTLY;
            case MKConfidenceLevel::UNCERTAIN:
                return ragAvailable ? MKEscalationPath::ESCALATE_RAG : MKEscalationPath::ESCALATE_CASCADE;
            case MKConfidenceLevel::VERY_UNCERTAIN:
                return meshAvailable ? MKEscalationPath::ESCALATE_MESH : MKEscalationPath::ESCALATE_CASCADE;
            case MKConfidenceLevel::UNKNOWN:
                return MKEscalationPath::MARK_UNKNOWN;
        }
        return MKEscalationPath::DEFER_TO_USER;
    }

public:
    MKConfidenceScorer() : certainThreshold(0.9f), confidentThreshold(0.7f),
                           uncertainThreshold(0.4f), veryUncertainThreshold(0.2f),
                           maxHistory(1000), totalEvaluations(0), escalations(0) {
        std::cout << "[CONFIDENCE] Uncertainty estimation engine ready.\n";
    }
    
    // Score confidence from model output logits
    MKConfidenceResult evaluate(const std::vector<float>& outputLogits,
                                bool ragAvailable = true, bool meshAvailable = false) {
        totalEvaluations++;
        MKConfidenceResult result;
        
        // Convert logits to probabilities
        std::vector<float> probs = softmax(outputLogits);
        
        // Entropy: low entropy = confident, high entropy = uncertain
        result.entropy = calculateEntropy(probs);
        float maxEntropy = std::log2((float)probs.size()); // Maximum possible entropy
        float normalizedEntropy = (maxEntropy > 0) ? result.entropy / maxEntropy : 1.0f;
        
        // Confidence = 1 - normalized_entropy (plus bonus for top-1 probability)
        float topProb = *std::max_element(probs.begin(), probs.end());
        result.score = (1.0f - normalizedEntropy) * 0.6f + topProb * 0.4f;
        result.score = std::max(0.0f, std::min(1.0f, result.score));
        
        // Determine level and escalation path
        result.level = scoreToLevel(result.score);
        result.path = determineEscalation(result.level, ragAvailable, meshAvailable);
        
        // Build reasoning string
        result.reasoning = "Entropy=" + std::to_string(result.entropy) + 
                          " | TopProb=" + std::to_string(topProb) +
                          " | Score=" + std::to_string(result.score);
        
        if (result.path != MKEscalationPath::ANSWER_DIRECTLY) escalations++;
        
        // Track history for calibration
        recentScores.push_back(result.score);
        if ((int)recentScores.size() > maxHistory) {
            recentScores.erase(recentScores.begin());
        }
        
        return result;
    }
    
    // Quick heuristic evaluation (no logits needed)
    MKConfidenceResult evaluateHeuristic(const std::string& query, bool hasRAGMatch,
                                          int knowledgeHits) {
        MKConfidenceResult result;
        result.entropy = 0.0f;
        
        float score = 0.5f; // Base
        
        // Boost confidence if RAG found matches
        if (hasRAGMatch) score += 0.25f;
        
        // Boost for knowledge base hits
        score += std::min(0.2f, knowledgeHits * 0.05f);
        
        // Reduce for question words (indicates need for reasoning)
        std::string lower = query;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower.find("why") != std::string::npos) score -= 0.15f;
        if (lower.find("how") != std::string::npos) score -= 0.1f;
        if (lower.find("explain") != std::string::npos) score -= 0.15f;
        if (lower.find("compare") != std::string::npos) score -= 0.2f;
        
        // Short factual queries get a boost
        if (query.size() < 30) score += 0.1f;
        
        result.score = std::max(0.0f, std::min(1.0f, score));
        result.level = scoreToLevel(result.score);
        result.path = determineEscalation(result.level, hasRAGMatch, false);
        result.reasoning = "Heuristic evaluation";
        
        totalEvaluations++;
        return result;
    }
    
    // Get average confidence across recent evaluations
    float getAverageConfidence() const {
        if (recentScores.empty()) return 0.0f;
        return std::accumulate(recentScores.begin(), recentScores.end(), 0.0f) / recentScores.size();
    }
    
    // Adjust thresholds based on hardware (lower thresholds on weak hardware = answers more)
    void adjustForHardware(int ramMB) {
        if (ramMB < 2048) {
            // On very low RAM, be more willing to answer directly (avoid heavy cascades)
            certainThreshold = 0.85f;
            confidentThreshold = 0.6f;
            uncertainThreshold = 0.3f;
        }
    }
    
    void printStats() const {
        std::cout << "[CONFIDENCE STATS] Evaluations: " << totalEvaluations
                  << " | Escalations: " << escalations
                  << " | Avg Score: " << getAverageConfidence() << "\n";
    }
};

#endif // MK_CONFIDENCE_CPP
