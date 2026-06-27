#ifndef MK_HONESTY_ENGINE_CPP
#define MK_HONESTY_ENGINE_CPP

#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <cstdlib>

// ===================================================================================
// MK HONESTY ENGINE - NEVER GUESS MODULE
// Core principle: MK never fabricates information. If confidence is below threshold,
// MK admits uncertainty honestly rather than guessing. This is what separates MK
// from systems that hallucinate.
// Features:
//   - Configurable confidence threshold (default 0.6)
//   - Multiple honest uncertainty response templates
//   - Tracks how often MK admits uncertainty (integrity metric)
//   - Suggests what MK can do instead (look it up, ask user, etc.)
//   - Integrates with MKConfidenceScorer thresholds
// ===================================================================================

enum class MKUncertaintyReason {
    LOW_CONFIDENCE,         // General low confidence score
    CONFLICTING_SOURCES,    // Sources disagree
    NO_SOURCES,             // Could not find any relevant sources
    STALE_DATA,             // Data is outdated
    OUTSIDE_KNOWLEDGE,      // Topic is outside MK's knowledge domain
    COMPLEX_QUERY           // Query requires reasoning MK cannot perform
};

struct MKHonestyResponse {
    bool shouldAnswer;              // Whether MK should attempt to answer
    std::string responseText;       // The honest response to give
    MKUncertaintyReason reason;     // Why MK is uncertain
    float confidenceGiven;          // The confidence that triggered this
    std::string suggestion;         // What MK suggests doing instead
};

class MKHonestyEngine {
private:
    float confidenceThreshold;      // Below this, MK admits uncertainty
    int totalQueries;
    int answeredQueries;
    int declinedQueries;
    int admittedUncertainty;

    // Get a human-friendly reason string
    std::string reasonToString(MKUncertaintyReason reason) {
        switch (reason) {
            case MKUncertaintyReason::LOW_CONFIDENCE:
                return "my confidence is too low to give a reliable answer";
            case MKUncertaintyReason::CONFLICTING_SOURCES:
                return "I found conflicting information from different sources";
            case MKUncertaintyReason::NO_SOURCES:
                return "I could not find any sources to verify this";
            case MKUncertaintyReason::STALE_DATA:
                return "my information on this topic may be outdated";
            case MKUncertaintyReason::OUTSIDE_KNOWLEDGE:
                return "this is outside my current knowledge base";
            case MKUncertaintyReason::COMPLEX_QUERY:
                return "this requires reasoning I cannot reliably perform";
        }
        return "I am not confident enough";
    }

    // Generate honest uncertainty responses
    std::string getUncertaintyTemplate(MKUncertaintyReason reason, const std::string& query) {
        switch (reason) {
            case MKUncertaintyReason::LOW_CONFIDENCE:
                return "I'm not confident enough to answer \"" + query +
                       "\" reliably. Let me look this up to give you accurate information.";

            case MKUncertaintyReason::CONFLICTING_SOURCES:
                return "I found conflicting information about \"" + query +
                       "\". Here's what different sources say, but I cannot determine which is correct.";

            case MKUncertaintyReason::NO_SOURCES:
                return "I don't have verified information about \"" + query +
                       "\". I'd rather admit this than guess and risk being wrong.";

            case MKUncertaintyReason::STALE_DATA:
                return "My information about \"" + query +
                       "\" may be outdated. Let me check for the latest data before answering.";

            case MKUncertaintyReason::OUTSIDE_KNOWLEDGE:
                return "\"" + query +
                       "\" is outside my current knowledge domain. I'll need to research this.";

            case MKUncertaintyReason::COMPLEX_QUERY:
                return "\"" + query +
                       "\" requires complex reasoning that I cannot perform with full confidence. "
                       "I can share what I know, but please verify independently.";
        }
        return "I'm not sure about this. Let me look it up rather than guess.";
    }

    // Suggest next steps
    std::string getSuggestion(MKUncertaintyReason reason) {
        switch (reason) {
            case MKUncertaintyReason::LOW_CONFIDENCE:
                return "I can search the internet for current, verified information.";
            case MKUncertaintyReason::CONFLICTING_SOURCES:
                return "I can present all perspectives and let you decide.";
            case MKUncertaintyReason::NO_SOURCES:
                return "I can search multiple sources to find an answer.";
            case MKUncertaintyReason::STALE_DATA:
                return "I can re-verify this with fresh data from the internet.";
            case MKUncertaintyReason::OUTSIDE_KNOWLEDGE:
                return "I can research this topic and add it to my knowledge base.";
            case MKUncertaintyReason::COMPLEX_QUERY:
                return "I can break this into simpler questions I can answer with confidence.";
        }
        return "I can look this up for you.";
    }

public:
    MKHonestyEngine() : confidenceThreshold(0.6f), totalQueries(0),
                        answeredQueries(0), declinedQueries(0), admittedUncertainty(0) {
        std::cout << "[HONESTY] Never-guess engine initialized. Threshold: "
                  << (int)(confidenceThreshold * 100) << "% confidence required.\n";
    }

    // Determine whether MK should attempt to answer given the confidence score
    bool shouldAnswer(float confidenceScore) {
        return confidenceScore >= confidenceThreshold;
    }

    // Get an honest response when MK cannot answer confidently
    MKHonestyResponse getHonestResponse(const std::string& query, float confidence,
                                         MKUncertaintyReason reason = MKUncertaintyReason::LOW_CONFIDENCE) {
        totalQueries++;
        MKHonestyResponse response;
        response.confidenceGiven = confidence;
        response.reason = reason;

        if (confidence >= confidenceThreshold) {
            // Confidence is above threshold - allow answering
            response.shouldAnswer = true;
            response.responseText = "";
            response.suggestion = "";
            answeredQueries++;
        } else {
            // Below threshold - provide honest uncertainty response
            response.shouldAnswer = false;
            response.responseText = getUncertaintyTemplate(reason, query);
            response.suggestion = getSuggestion(reason);
            declinedQueries++;
            admittedUncertainty++;

            std::cout << "[HONESTY] Declined to answer: \"" << query
                      << "\" (confidence: " << (int)(confidence * 100) << "%, reason: "
                      << reasonToString(reason) << ")\n";
        }

        return response;
    }

    // Explicitly admit uncertainty for a query
    MKHonestyResponse admitUncertainty(const std::string& query) {
        return getHonestResponse(query, 0.0f, MKUncertaintyReason::LOW_CONFIDENCE);
    }

    // Admit uncertainty with a specific reason
    MKHonestyResponse admitUncertaintyWithReason(const std::string& query,
                                                  MKUncertaintyReason reason) {
        return getHonestResponse(query, 0.0f, reason);
    }

    // Get the current confidence threshold
    float getThreshold() const { return confidenceThreshold; }

    // Adjust the confidence threshold
    void setThreshold(float threshold) {
        confidenceThreshold = std::max(0.1f, std::min(0.95f, threshold));
        std::cout << "[HONESTY] Threshold updated to: " << (int)(confidenceThreshold * 100) << "%\n";
    }

    // Get honesty statistics
    float getHonestyRate() const {
        if (totalQueries == 0) return 1.0f;
        return (float)admittedUncertainty / (float)totalQueries;
    }

    void printStats() const {
        std::cout << "[HONESTY STATS] Total queries: " << totalQueries
                  << " | Answered: " << answeredQueries
                  << " | Declined: " << declinedQueries
                  << " | Honesty rate: " << (int)(getHonestyRate() * 100) << "%\n";
    }
};

#endif // MK_HONESTY_ENGINE_CPP
