#ifndef MK_KNOWLEDGE_INTEGRATOR_CPP
#define MK_KNOWLEDGE_INTEGRATOR_CPP

#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <sstream>

#include "../network/search_engine.cpp"
#include "fact_verifier.cpp"
#include "freshness_manager.cpp"
#include "cited_response.cpp"
#include "honesty_engine.cpp"

// ===================================================================================
// MK KNOWLEDGE INTEGRATOR - UNIFIED COORDINATION MODULE
// Ties together all accuracy-first modules into a single coherent workflow:
//   - MKSearchEngine: Find information on the internet
//   - MKFactVerifier: Cross-reference and verify facts
//   - MKFreshnessManager: Track staleness and expiry of knowledge
//   - MKCitedResponse: Build fully-cited answers with provenance
//   - MKHonestyEngine: Never guess, admit uncertainty honestly
//
// Provides high-level workflows:
//   - learnFromInternet: Full pipeline (search -> verify -> store with freshness)
//   - refreshStaleKnowledge: Re-verify facts that have gone stale
//   - buildCitedAnswer: Search + verify + cite with honesty check
//   - getSystemStatus: Dashboard of all module states
// ===================================================================================

struct MKLearnedFact {
    std::string text;
    float confidence;
    std::string source;
    std::time_t learnedAt;
    bool verified;
};

struct MKIntegratorStats {
    int totalLearningCycles;
    int factsLearned;
    int factsRejected;
    int staleRefreshed;
    int answersBuilt;
    int answersDeclined;
};

class MKKnowledgeIntegrator {
private:
    MKSearchEngine searchEngine;
    MKFactVerifier factVerifier;
    MKFreshnessManager freshnessManager;
    MKCitedResponse citedResponse;
    MKHonestyEngine honestyEngine;

    std::vector<MKLearnedFact> learnedFacts;
    MKIntegratorStats stats;

    // Generate a fact key for freshness tracking
    std::string generateFactKey(const std::string& fact) {
        // Simple hash-like key from first 50 chars
        std::string key = fact.substr(0, std::min((size_t)50, fact.size()));
        // Replace spaces with underscores for a clean key
        for (char& c : key) {
            if (c == ' ') c = '_';
            else if (!std::isalnum(c) && c != '_') c = '-';
        }
        return key;
    }

public:
    MKKnowledgeIntegrator() {
        stats.totalLearningCycles = 0;
        stats.factsLearned = 0;
        stats.factsRejected = 0;
        stats.staleRefreshed = 0;
        stats.answersBuilt = 0;
        stats.answersDeclined = 0;

        std::cout << "[INTEGRATOR] Knowledge integration engine initialized.\n";
        std::cout << "[INTEGRATOR] Modules online: SearchEngine, FactVerifier, "
                  << "FreshnessManager, CitedResponse, HonestyEngine\n";
    }

    // Full learning pipeline: search the internet -> verify each fact -> store with freshness
    std::vector<MKLearnedFact> learnFromInternet(const std::string& topic) {
        stats.totalLearningCycles++;
        std::vector<MKLearnedFact> results;

        std::cout << "[INTEGRATOR] Learning pipeline started for: \"" << topic << "\"\n";

        // Step 1: Search for information
        auto searchResults = searchEngine.searchMultiple(topic);
        std::cout << "[INTEGRATOR] Search returned " << searchResults.size() << " results.\n";

        if (searchResults.empty()) {
            std::cout << "[INTEGRATOR] No search results found. Cannot learn about: "
                      << topic << "\n";
            return results;
        }

        // Step 2: Extract candidate facts from search snippets
        std::vector<std::string> candidates;
        for (const auto& result : searchResults) {
            if (!result.snippet.empty() && result.snippet.size() > 20) {
                candidates.push_back(result.snippet);
            }
        }

        // Step 3: Verify each candidate fact
        for (const auto& candidate : candidates) {
            auto verification = factVerifier.verify(candidate);

            MKLearnedFact fact;
            fact.text = candidate;
            fact.confidence = verification.confidence;
            fact.source = (!verification.sourceUrls.empty()) ? verification.sourceUrls[0] : "search";
            fact.learnedAt = std::time(nullptr);

            if (verification.confidence >= 0.5f) {
                // Fact passes verification threshold
                fact.verified = true;
                results.push_back(fact);
                learnedFacts.push_back(fact);
                stats.factsLearned++;

                // Step 4: Register with freshness manager
                std::string factKey = generateFactKey(candidate);
                freshnessManager.registerFact(factKey, topic);

                std::cout << "[INTEGRATOR] LEARNED: \"" << candidate.substr(0, 60)
                          << "...\" (confidence: " << (int)(verification.confidence * 100) << "%)\n";
            } else {
                // Fact did not pass verification
                fact.verified = false;
                stats.factsRejected++;

                std::cout << "[INTEGRATOR] REJECTED: \"" << candidate.substr(0, 60)
                          << "...\" (confidence too low: "
                          << (int)(verification.confidence * 100) << "%)\n";
            }
        }

        std::cout << "[INTEGRATOR] Learning cycle complete. Learned: " << results.size()
                  << " | Rejected: " << (candidates.size() - results.size()) << "\n";
        return results;
    }

    // Find stale facts via MKFreshnessManager and re-verify them
    int refreshStaleKnowledge() {
        std::cout << "[INTEGRATOR] Refreshing stale knowledge...\n";

        auto staleFacts = freshnessManager.getStaleFactsList();
        int refreshed = 0;

        if (staleFacts.empty()) {
            std::cout << "[INTEGRATOR] No stale facts found. Knowledge is fresh.\n";
            return 0;
        }

        std::cout << "[INTEGRATOR] Found " << staleFacts.size() << " stale facts to refresh.\n";

        for (const auto& factKey : staleFacts) {
            // Search for updated information about this fact
            auto searchResults = searchEngine.search(factKey);

            if (!searchResults.empty()) {
                // Re-verify using the first relevant snippet
                auto verification = factVerifier.verify(searchResults[0].snippet);

                if (verification.confidence >= 0.5f) {
                    // Fact is still valid - mark as freshly verified
                    freshnessManager.markVerified(factKey);
                    refreshed++;
                    stats.staleRefreshed++;

                    std::cout << "[INTEGRATOR] Refreshed: " << factKey
                              << " (still valid, confidence: "
                              << (int)(verification.confidence * 100) << "%)\n";
                } else {
                    // Fact may no longer be accurate
                    freshnessManager.flagForReverification(factKey);
                    std::cout << "[INTEGRATOR] WARNING: " << factKey
                              << " may be outdated (low confidence on re-check)\n";
                }
            }
        }

        std::cout << "[INTEGRATOR] Refresh complete. " << refreshed << "/"
                  << staleFacts.size() << " facts confirmed still valid.\n";
        return refreshed;
    }

    // Full cited answer pipeline: search + verify + build response with honesty check
    MKCitedAnswer buildCitedAnswer(const std::string& query) {
        std::cout << "[INTEGRATOR] Building cited answer for: \"" << query << "\"\n";

        // Step 1: Search for information
        auto searchResults = searchEngine.searchMultiple(query);

        if (searchResults.empty()) {
            // No results - use honesty engine to admit uncertainty
            auto honesty = honestyEngine.admitUncertainty(query);
            stats.answersDeclined++;

            MKCitedAnswer emptyAnswer;
            emptyAnswer.text = honesty.responseText;
            emptyAnswer.confidence = 0.0f;
            emptyAnswer.agreeingCount = 0;
            emptyAnswer.verifiedAt = std::time(nullptr);
            emptyAnswer.formattedResponse = honesty.responseText + "\n" + honesty.suggestion;
            return emptyAnswer;
        }

        // Step 2: Take the best result and verify it
        std::string bestSnippet = searchResults[0].snippet;
        auto verification = factVerifier.verify(bestSnippet);

        // Step 3: Honesty check - should we answer?
        auto honesty = honestyEngine.getHonestResponse(query, verification.confidence);

        if (!honesty.shouldAnswer) {
            // Confidence too low - admit uncertainty
            stats.answersDeclined++;

            MKCitedAnswer lowConfAnswer;
            lowConfAnswer.text = honesty.responseText;
            lowConfAnswer.confidence = verification.confidence;
            lowConfAnswer.agreeingCount = verification.agreeingSources;
            lowConfAnswer.verifiedAt = verification.verifiedAt;
            lowConfAnswer.formattedResponse = honesty.responseText + "\n" + honesty.suggestion;

            std::cout << "[INTEGRATOR] Answer declined (confidence too low: "
                      << (int)(verification.confidence * 100) << "%)\n";
            return lowConfAnswer;
        }

        // Step 4: Build the cited response
        stats.answersBuilt++;

        auto answer = citedResponse.buildResponse(
            bestSnippet,
            verification.confidence,
            verification.sourceUrls,
            verification.agreeingSources,
            verification.verifiedAt
        );

        std::cout << "[INTEGRATOR] Cited answer built. Confidence: "
                  << (int)(verification.confidence * 100) << "%, Sources: "
                  << verification.sourceUrls.size() << "\n";

        return answer;
    }

    // Get a comprehensive system status string showing all module states
    std::string getSystemStatus() {
        std::ostringstream status;
        status << "=== MK Knowledge Integrator - System Status ===\n\n";

        status << "[ Search Engine ]\n";
        status << "  Total searches: " << searchEngine.getTotalSearches() << "\n";
        status << "  Successful: " << searchEngine.getSuccessfulSearches() << "\n\n";

        status << "[ Fact Verifier ]\n";
        status << "  Agreement threshold: "
               << (int)(factVerifier.getAgreementThreshold() * 100) << "%\n\n";

        status << "[ Freshness Manager ]\n";
        auto staleFacts = freshnessManager.getStaleFactsList();
        status << "  Stale facts needing refresh: " << staleFacts.size() << "\n\n";

        status << "[ Honesty Engine ]\n";
        status << "  Confidence threshold: "
               << (int)(honestyEngine.getThreshold() * 100) << "%\n\n";

        status << "[ Integration Stats ]\n";
        status << "  Learning cycles: " << stats.totalLearningCycles << "\n";
        status << "  Facts learned: " << stats.factsLearned << "\n";
        status << "  Facts rejected: " << stats.factsRejected << "\n";
        status << "  Stale refreshed: " << stats.staleRefreshed << "\n";
        status << "  Answers built: " << stats.answersBuilt << "\n";
        status << "  Answers declined (honesty): " << stats.answersDeclined << "\n";
        status << "  Total learned facts in memory: " << learnedFacts.size() << "\n";
        status << "================================================\n";

        return status.str();
    }

    // Get all learned facts
    const std::vector<MKLearnedFact>& getLearnedFacts() const {
        return learnedFacts;
    }

    // Get stats
    const MKIntegratorStats& getStats() const {
        return stats;
    }

    void printStats() const {
        std::cout << "[INTEGRATOR STATS] Cycles: " << stats.totalLearningCycles
                  << " | Learned: " << stats.factsLearned
                  << " | Rejected: " << stats.factsRejected
                  << " | Refreshed: " << stats.staleRefreshed
                  << " | Answers: " << stats.answersBuilt
                  << " | Declined: " << stats.answersDeclined << "\n";
    }
};

#endif // MK_KNOWLEDGE_INTEGRATOR_CPP
