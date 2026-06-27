#ifndef MK_REALTIME_SEARCH_CPP
#define MK_REALTIME_SEARCH_CPP

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <numeric>

// ===========================================================================
// MK REALTIME SEARCH - Multi-Source Answer Synthesis
// ===========================================================================
// Combines DuckDuckGo, Wikipedia, and web scraping for comprehensive answers.
// Implements query decomposition, result merging, source credibility scoring,
// and answer synthesis from multiple sources.
// ===========================================================================

// A single search result from any source
struct MKSearchHit {
    std::string title;
    std::string snippet;
    std::string url;
    std::string source;           // duckduckgo, wikipedia, web
    float relevance_score;        // 0.0 to 1.0
    float credibility_score;      // 0.0 to 1.0
    float combined_score;         // weighted combination
};

// Decomposed sub-query
struct MKSubQuery {
    std::string query;
    std::string intent;           // fact, definition, comparison, howto
    float importance;             // how important to the full answer
    std::vector<MKSearchHit> results;
    bool answered;
};

// Synthesized answer from multiple sources
struct MKSynthesizedAnswer {
    bool success;
    std::string answer;           // Final composed answer
    std::string confidence_level; // high, medium, low
    float confidence_score;       // 0.0 to 1.0
    int sources_used;
    std::vector<std::string> citations;
    std::vector<MKSearchHit> all_results;
    std::string query_decomposition;  // How the query was broken down
};

class MKRealtimeSearch {
private:
    // Source credibility weights
    std::unordered_map<std::string, float> source_credibility;
    int total_searches;
    int successful_answers;

    void initCredibility() {
        source_credibility["wikipedia"] = 0.85f;
        source_credibility["duckduckgo"] = 0.7f;
        source_credibility["stackoverflow"] = 0.8f;
        source_credibility["github"] = 0.75f;
        source_credibility["docs"] = 0.9f;
        source_credibility["news"] = 0.6f;
        source_credibility["blog"] = 0.5f;
        source_credibility["forum"] = 0.4f;
        source_credibility["web"] = 0.5f;
    }

    // Decompose a complex query into sub-queries
    std::vector<MKSubQuery> decomposeQuery(const std::string& query) {
        std::vector<MKSubQuery> sub_queries;
        std::string lower = query;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        // Check for comparison queries
        if (lower.find(" vs ") != std::string::npos || lower.find("compare") != std::string::npos ||
            lower.find("difference between") != std::string::npos) {
            // Split into parts being compared
            size_t vs_pos = lower.find(" vs ");
            if (vs_pos == std::string::npos) vs_pos = lower.find(" and ");
            if (vs_pos != std::string::npos) {
                std::string part1 = query.substr(0, vs_pos);
                std::string part2 = query.substr(vs_pos + 4);
                sub_queries.push_back({"What is " + part1, "definition", 0.5f, {}, false});
                sub_queries.push_back({"What is " + part2, "definition", 0.5f, {}, false});
                sub_queries.push_back({query, "comparison", 1.0f, {}, false});
            }
        }
        // Multi-part questions (contains 'and')
        else if (lower.find(" and ") != std::string::npos && lower.find("?") != std::string::npos) {
            std::stringstream ss(query);
            std::string part;
            while (std::getline(ss, part, '?')) {
                if (part.length() > 5) {
                    sub_queries.push_back({part + "?", "fact", 1.0f, {}, false});
                }
            }
        }
        // Single question - use as-is
        else {
            std::string intent = "fact";
            if (lower.find("how to") != std::string::npos || lower.find("how do") != std::string::npos)
                intent = "howto";
            else if (lower.find("what is") != std::string::npos || lower.find("define") != std::string::npos)
                intent = "definition";
            sub_queries.push_back({query, intent, 1.0f, {}, false});
        }

        if (sub_queries.empty()) {
            sub_queries.push_back({query, "fact", 1.0f, {}, false});
        }
        return sub_queries;
    }

    // Score relevance of a result to a query
    float scoreRelevance(const std::string& query, const MKSearchHit& hit) {
        std::string lower_query = query;
        std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);

        std::string lower_snippet = hit.snippet;
        std::transform(lower_snippet.begin(), lower_snippet.end(), lower_snippet.begin(), ::tolower);

        // Keyword overlap scoring
        std::vector<std::string> query_words;
        std::stringstream ss(lower_query);
        std::string word;
        while (ss >> word) {
            if (word.length() > 3) query_words.push_back(word);
        }

        int matches = 0;
        for (const auto& qw : query_words) {
            if (lower_snippet.find(qw) != std::string::npos) matches++;
        }

        float relevance = query_words.empty() ? 0.0f :
            static_cast<float>(matches) / query_words.size();

        // Boost for title match
        std::string lower_title = hit.title;
        std::transform(lower_title.begin(), lower_title.end(), lower_title.begin(), ::tolower);
        for (const auto& qw : query_words) {
            if (lower_title.find(qw) != std::string::npos) relevance += 0.1f;
        }

        return std::min(1.0f, relevance);
    }

    // Merge and deduplicate results from multiple sources
    std::vector<MKSearchHit> mergeResults(const std::vector<std::vector<MKSearchHit>>& all_results) {
        std::vector<MKSearchHit> merged;
        std::unordered_set<std::string> seen_urls;

        for (const auto& results : all_results) {
            for (const auto& hit : results) {
                if (seen_urls.find(hit.url) == seen_urls.end()) {
                    merged.push_back(hit);
                    seen_urls.insert(hit.url);
                }
            }
        }

        // Sort by combined score
        std::sort(merged.begin(), merged.end(),
                  [](const MKSearchHit& a, const MKSearchHit& b) {
                      return a.combined_score > b.combined_score;
                  });

        return merged;
    }

    // Synthesize an answer from top results
    std::string synthesizeAnswer(const std::string& query,
                                  const std::vector<MKSearchHit>& results) {
        if (results.empty()) return "I could not find relevant information.";

        std::string answer;
        // Use the top result's snippet as the primary answer
        if (!results.empty()) {
            answer = results[0].snippet;
        }

        // Add supporting info from other sources
        if (results.size() > 1 && results[1].combined_score > 0.5f) {
            answer += " Additionally, " + results[1].snippet;
        }

        return answer;
    }

public:
    MKRealtimeSearch() : total_searches(0), successful_answers(0) {
        initCredibility();
    }

    // Main search method - takes a query, returns synthesized answer
    MKSynthesizedAnswer search(const std::string& query,
                                const std::vector<MKSearchHit>& external_results = {}) {
        MKSynthesizedAnswer answer;
        answer.success = false;
        total_searches++;

        // Decompose query
        auto sub_queries = decomposeQuery(query);
        std::string decomp;
        for (const auto& sq : sub_queries) {
            decomp += "[" + sq.intent + "] " + sq.query + "\n";
        }
        answer.query_decomposition = decomp;

        // Score external results
        std::vector<MKSearchHit> scored_results;
        for (auto hit : external_results) {
            hit.relevance_score = scoreRelevance(query, hit);
            auto cred_it = source_credibility.find(hit.source);
            hit.credibility_score = (cred_it != source_credibility.end()) ?
                cred_it->second : 0.5f;
            hit.combined_score = hit.relevance_score * 0.6f + hit.credibility_score * 0.4f;
            scored_results.push_back(hit);
        }

        // Sort by combined score
        std::sort(scored_results.begin(), scored_results.end(),
                  [](const MKSearchHit& a, const MKSearchHit& b) {
                      return a.combined_score > b.combined_score;
                  });

        answer.all_results = scored_results;

        // Synthesize answer from top results
        if (!scored_results.empty()) {
            answer.answer = synthesizeAnswer(query, scored_results);
            answer.success = true;
            answer.sources_used = std::min(3, static_cast<int>(scored_results.size()));

            // Add citations
            for (int i = 0; i < answer.sources_used; i++) {
                answer.citations.push_back(scored_results[i].url);
            }

            // Confidence based on scores
            float avg_score = 0;
            for (int i = 0; i < answer.sources_used; i++)
                avg_score += scored_results[i].combined_score;
            avg_score /= answer.sources_used;
            answer.confidence_score = avg_score;

            if (avg_score > 0.7f) answer.confidence_level = "high";
            else if (avg_score > 0.4f) answer.confidence_level = "medium";
            else answer.confidence_level = "low";

            successful_answers++;
        } else {
            answer.answer = "No relevant results found for: " + query;
            answer.confidence_score = 0.0f;
            answer.confidence_level = "none";
        }

        return answer;
    }

    // Get source credibility for a domain
    float getCredibility(const std::string& source) const {
        auto it = source_credibility.find(source);
        return (it != source_credibility.end()) ? it->second : 0.5f;
    }

    int getTotalSearches() const { return total_searches; }
    int getSuccessful() const { return successful_answers; }

    void printStats() const {
        std::cout << "[MKRealtimeSearch] Searches: " << total_searches
                  << ", Answered: " << successful_answers << std::endl;
    }
};

#endif // MK_REALTIME_SEARCH_CPP
