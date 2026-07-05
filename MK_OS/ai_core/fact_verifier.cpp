// LEGACY: This file is not included in the main build or test suite.
// Only referenced by network/autonomous_agent.cpp (also legacy).
// Fact validation is now handled by mind/knowledge_validator.cpp.
#ifndef MK_FACT_VERIFIER_CPP
#define MK_FACT_VERIFIER_CPP

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <ctime>

#include "../network/search_engine.cpp"

// ===================================================================================
// MK FACT VERIFIER - CROSS-REFERENCE ENGINE
// Takes a claimed fact, queries multiple internet sources via MKSearchEngine,
// scores confidence based on source agreement, and detects contradictions.
// This is the core of MK's accuracy-first philosophy: never guess, always verify.
// Features:
//   - Multi-source fact cross-referencing
//   - Agreement scoring (more sources agreeing = higher confidence)
//   - Contradiction detection (sources disagreeing flags low confidence)
//   - Verification result with full provenance (source URLs, timestamps)
//   - Configurable verification thresholds
// ===================================================================================

struct MKContradiction {
    std::string sourceA;
    std::string claimA;
    std::string sourceB;
    std::string claimB;
    std::string description;
};

struct MKVerificationResult {
    float confidence;                       // 0.0 to 1.0 overall confidence
    int agreeingSources;                    // How many sources support the claim
    int totalSourcesChecked;                // How many sources were queried
    std::vector<MKContradiction> contradictions;   // Detected contradictions
    std::vector<std::string> sourceUrls;    // URLs that were checked
    std::vector<std::string> supportingSnippets;   // Text that supports the claim
    std::time_t verifiedAt;                 // Timestamp of verification
    std::string summary;                    // Human-readable summary
};

class MKFactVerifier {
private:
    MKSearchEngine searchEngine;
    int totalVerifications;
    int highConfidenceCount;
    int contradictionCount;

    // Minimum agreement threshold for a fact to be considered verified
    float agreementThreshold;

    // Convert a string to lowercase for case-insensitive comparison
    std::string toLower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        return s;
    }

    // Split a string into individual words for keyword matching
    std::vector<std::string> tokenize(const std::string& text) {
        std::vector<std::string> tokens;
        std::string current;
        for (char c : text) {
            if (std::isalnum(c)) {
                current += std::tolower(c);
            } else if (!current.empty()) {
                if (current.size() > 2) { // Skip very short words
                    tokens.push_back(current);
                }
                current.clear();
            }
        }
        if (!current.empty() && current.size() > 2) tokens.push_back(current);
        return tokens;
    }

    // Calculate keyword overlap between claim and a source snippet
    float calculateOverlap(const std::string& claim, const std::string& snippet) {
        auto claimTokens = tokenize(claim);
        auto snippetTokens = tokenize(snippet);

        if (claimTokens.empty()) return 0.0f;

        int matches = 0;
        for (const auto& ct : claimTokens) {
            for (const auto& st : snippetTokens) {
                if (ct == st) {
                    matches++;
                    break;
                }
            }
        }
        return (float)matches / (float)claimTokens.size();
    }

    // Detect if two snippets contradict each other regarding the claim
    bool detectContradiction(const std::string& snippetA, const std::string& snippetB,
                             const std::string& claim) {
        // Look for negation patterns that differ between sources
        std::vector<std::string> negationWords = {"not", "never", "false", "incorrect",
                                                   "wrong", "untrue", "myth", "disproven",
                                                   "no longer", "wasn't", "isn't", "aren't"};

        std::string lowerA = toLower(snippetA);
        std::string lowerB = toLower(snippetB);

        int negationsA = 0, negationsB = 0;
        for (const auto& neg : negationWords) {
            if (lowerA.find(neg) != std::string::npos) negationsA++;
            if (lowerB.find(neg) != std::string::npos) negationsB++;
        }

        // If one source has negations and the other doesn't, it's a potential contradiction
        // Both must be relevant to the claim to count
        float relevanceA = calculateOverlap(claim, snippetA);
        float relevanceB = calculateOverlap(claim, snippetB);

        if (relevanceA > 0.3f && relevanceB > 0.3f) {
            if ((negationsA > 0) != (negationsB > 0)) {
                return true;
            }
        }
        return false;
    }

public:
    MKFactVerifier() : totalVerifications(0), highConfidenceCount(0),
                       contradictionCount(0), agreementThreshold(0.5f) {
        std::cout << "[VERIFIER] Fact verification cross-reference engine initialized.\n";
    }

    // Verify a claimed fact by cross-referencing multiple internet sources
    MKVerificationResult verify(const std::string& claim) {
        totalVerifications++;
        MKVerificationResult result;
        result.verifiedAt = std::time(nullptr);
        result.agreeingSources = 0;
        result.totalSourcesChecked = 0;

        // Search multiple sources for information about this claim
        auto searchResults = searchEngine.searchMultiple(claim);
        result.totalSourcesChecked = (int)searchResults.size();

        if (searchResults.empty()) {
            result.confidence = 0.0f;
            result.summary = "No sources found to verify this claim.";
            std::cout << "[VERIFIER] Claim: \"" << claim << "\" -> NO SOURCES FOUND\n";
            return result;
        }

        // Analyze each source for agreement/contradiction
        std::vector<std::string> relevantSnippets;
        for (const auto& sr : searchResults) {
            float overlap = calculateOverlap(claim, sr.snippet);
            result.sourceUrls.push_back(sr.url);

            if (overlap > 0.3f) {
                // This source is relevant to the claim
                result.agreeingSources++;
                result.supportingSnippets.push_back(sr.snippet);
                relevantSnippets.push_back(sr.snippet);
            }
        }

        // Cross-check relevant snippets for contradictions
        for (size_t i = 0; i < relevantSnippets.size(); i++) {
            for (size_t j = i + 1; j < relevantSnippets.size(); j++) {
                if (detectContradiction(relevantSnippets[i], relevantSnippets[j], claim)) {
                    MKContradiction c;
                    c.sourceA = (i < result.sourceUrls.size()) ? result.sourceUrls[i] : "unknown";
                    c.claimA = relevantSnippets[i].substr(0, 100);
                    c.sourceB = (j < result.sourceUrls.size()) ? result.sourceUrls[j] : "unknown";
                    c.claimB = relevantSnippets[j].substr(0, 100);
                    c.description = "Sources disagree on: " + claim;
                    result.contradictions.push_back(c);
                    contradictionCount++;
                }
            }
        }

        // Calculate confidence score
        float agreementRatio = (result.totalSourcesChecked > 0)
            ? (float)result.agreeingSources / (float)result.totalSourcesChecked
            : 0.0f;

        float contradictionPenalty = std::min(0.4f, (float)result.contradictions.size() * 0.2f);
        float sourceBonus = std::min(0.2f, (float)result.agreeingSources * 0.05f);

        result.confidence = std::max(0.0f, std::min(1.0f,
            agreementRatio * 0.7f + sourceBonus - contradictionPenalty));

        if (result.confidence >= 0.7f) highConfidenceCount++;

        // Build summary
        result.summary = "Checked " + std::to_string(result.totalSourcesChecked) + " sources. "
                       + std::to_string(result.agreeingSources) + " agree. "
                       + std::to_string(result.contradictions.size()) + " contradictions. "
                       + "Confidence: " + std::to_string((int)(result.confidence * 100)) + "%.";

        std::cout << "[VERIFIER] Claim: \"" << claim << "\" -> "
                  << result.summary << "\n";

        return result;
    }

    // Get the agreement threshold
    float getAgreementThreshold() const { return agreementThreshold; }

    // Adjust thresholds
    void setAgreementThreshold(float threshold) {
        agreementThreshold = std::max(0.1f, std::min(0.9f, threshold));
    }

    void printStats() const {
        std::cout << "[VERIFIER STATS] Total verifications: " << totalVerifications
                  << " | High confidence: " << highConfidenceCount
                  << " | Contradictions found: " << contradictionCount << "\n";
    }
};

#endif // MK_FACT_VERIFIER_CPP
