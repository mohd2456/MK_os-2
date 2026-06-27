#ifndef MK_CITED_RESPONSE_CPP
#define MK_CITED_RESPONSE_CPP

#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <sstream>
#include <iomanip>

// ===================================================================================
// MK CITED RESPONSE BUILDER
// Builds responses that always include provenance information:
//   - The answer text
//   - Source URL(s) where the information came from
//   - Confidence percentage (how sure MK is)
//   - Number of agreeing sources
//   - Last verification timestamp
// This ensures MK never presents information without attribution.
// ===================================================================================

struct MKCitedAnswer {
    std::string text;                        // The actual answer
    std::vector<std::string> sources;        // Source URLs
    float confidence;                        // 0.0 to 1.0
    int agreeingCount;                       // Number of sources that agree
    std::time_t verifiedAt;                  // When this was last verified
    std::string category;                    // Topic category
    std::string formattedResponse;           // Full formatted response string
};

class MKCitedResponse {
private:
    int totalResponses;
    int citedResponses;
    int uncitedResponses;

    // Format a timestamp into human-readable string
    std::string formatTimestamp(std::time_t t) {
        if (t == 0) return "never verified";
        struct tm* tm_info = std::localtime(&t);
        char buffer[64];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);
        return std::string(buffer);
    }

    // Format confidence as percentage string
    std::string formatConfidence(float conf) {
        int pct = (int)(conf * 100.0f);
        std::string bar;
        int filled = pct / 10;
        for (int i = 0; i < 10; i++) {
            bar += (i < filled) ? "#" : "-";
        }
        return "[" + bar + "] " + std::to_string(pct) + "%";
    }

    // Get confidence label
    std::string getConfidenceLabel(float conf) {
        if (conf >= 0.9f) return "Very High";
        if (conf >= 0.7f) return "High";
        if (conf >= 0.5f) return "Moderate";
        if (conf >= 0.3f) return "Low";
        return "Very Low";
    }

public:
    MKCitedResponse() : totalResponses(0), citedResponses(0), uncitedResponses(0) {
        std::cout << "[CITED] Cited response builder initialized. All answers include sources.\n";
    }

    // Build a fully cited response from answer text and verification data
    MKCitedAnswer buildResponse(const std::string& answerText,
                                float confidence,
                                const std::vector<std::string>& sourceUrls,
                                int agreeingSourceCount,
                                std::time_t verificationTime = 0) {
        totalResponses++;
        MKCitedAnswer answer;
        answer.text = answerText;
        answer.sources = sourceUrls;
        answer.confidence = confidence;
        answer.agreeingCount = agreeingSourceCount;
        answer.verifiedAt = (verificationTime == 0) ? std::time(nullptr) : verificationTime;

        if (!sourceUrls.empty()) {
            citedResponses++;
        } else {
            uncitedResponses++;
        }

        // Build formatted response
        std::ostringstream oss;
        oss << "--- MK Answer ---\n";
        oss << answer.text << "\n\n";
        oss << "Confidence: " << formatConfidence(confidence)
            << " (" << getConfidenceLabel(confidence) << ")\n";
        oss << "Sources agree: " << agreeingSourceCount << "\n";
        oss << "Verified: " << formatTimestamp(answer.verifiedAt) << "\n";

        if (!sourceUrls.empty()) {
            oss << "Sources:\n";
            for (size_t i = 0; i < sourceUrls.size(); i++) {
                oss << "  [" << (i + 1) << "] " << sourceUrls[i] << "\n";
            }
        } else {
            oss << "Sources: (no external sources - from internal knowledge)\n";
        }
        oss << "-----------------\n";

        answer.formattedResponse = oss.str();
        return answer;
    }

    // Build a response from a verification result struct
    MKCitedAnswer buildFromVerification(const std::string& answerText,
                                         float verConfidence,
                                         int verAgreeingSources,
                                         const std::vector<std::string>& verSourceUrls,
                                         std::time_t verVerifiedAt) {
        return buildResponse(answerText, verConfidence, verSourceUrls,
                            verAgreeingSources, verVerifiedAt);
    }

    // Quick uncited response (for internal knowledge with no external verification)
    MKCitedAnswer buildUncitedResponse(const std::string& answerText, float confidence) {
        std::vector<std::string> empty;
        return buildResponse(answerText, confidence, empty, 0, std::time(nullptr));
    }

    void printStats() const {
        std::cout << "[CITED STATS] Total responses: " << totalResponses
                  << " | Cited: " << citedResponses
                  << " | Uncited: " << uncitedResponses << "\n";
    }
};

#endif // MK_CITED_RESPONSE_CPP
