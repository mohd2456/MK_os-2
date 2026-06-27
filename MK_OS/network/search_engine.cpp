#ifndef MK_SEARCH_ENGINE_CPP
#define MK_SEARCH_ENGINE_CPP

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <map>

#include "http.cpp"

// ===================================================================================
// MK MULTI-SOURCE INTERNET SEARCH ENGINE
// Queries multiple APIs (DuckDuckGo Instant Answer, Wikipedia REST) to gather
// information from diverse sources. Includes a lightweight JSON parser that
// extracts key fields without requiring an external JSON library.
// Features:
//   - DuckDuckGo Instant Answer API queries
//   - Wikipedia page summary API queries
//   - Multi-source aggregation with deduplication
//   - Lightweight JSON string/array extraction
//   - Result ranking by relevance and source trust
// ===================================================================================

struct MKSearchResult {
    std::string title;
    std::string snippet;
    std::string url;
    std::string source;       // "duckduckgo", "wikipedia", etc.
    float relevanceScore;     // 0.0 to 1.0
};

class MKSearchEngine {
private:
    MKHTTP http;
    int totalSearches;
    int successfulSearches;

    // URL-encode a query string for use in API calls
    std::string urlEncode(const std::string& input) {
        std::string encoded;
        for (char c : input) {
            if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                encoded += c;
            } else if (c == ' ') {
                encoded += '+';
            } else {
                char hex[4];
                std::snprintf(hex, sizeof(hex), "%%%02X", (unsigned char)c);
                encoded += hex;
            }
        }
        return encoded;
    }

    // ─── Lightweight JSON Helpers ───────────────────────────────────────────────

    // Extract the value for a given key from a JSON object string.
    // Handles string values (returns unquoted) and falls back to raw content.
    std::string jsonGetString(const std::string& json, const std::string& key) {
        std::string search = "\"" + key + "\"";
        size_t pos = json.find(search);
        if (pos == std::string::npos) return "";

        pos += search.size();
        // Skip whitespace and colon
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == ':' || json[pos] == '\t')) pos++;

        if (pos >= json.size()) return "";

        if (json[pos] == '"') {
            // String value - extract until unescaped closing quote
            pos++; // skip opening quote
            std::string value;
            while (pos < json.size()) {
                if (json[pos] == '\\' && pos + 1 < json.size()) {
                    pos++;
                    if (json[pos] == '"') value += '"';
                    else if (json[pos] == 'n') value += '\n';
                    else if (json[pos] == 't') value += '\t';
                    else if (json[pos] == '\\') value += '\\';
                    else if (json[pos] == '/') value += '/';
                    else value += json[pos];
                } else if (json[pos] == '"') {
                    break;
                } else {
                    value += json[pos];
                }
                pos++;
            }
            return value;
        }

        // Non-string value (number, bool, null) - read until comma/brace
        std::string value;
        while (pos < json.size() && json[pos] != ',' && json[pos] != '}' && json[pos] != ']') {
            value += json[pos];
            pos++;
        }
        // Trim whitespace
        while (!value.empty() && (value.back() == ' ' || value.back() == '\n' || value.back() == '\r'))
            value.pop_back();
        return value;
    }

    // Extract an array of JSON objects from a key (returns the raw array contents)
    std::vector<std::string> jsonGetArray(const std::string& json, const std::string& key) {
        std::vector<std::string> items;
        std::string search = "\"" + key + "\"";
        size_t pos = json.find(search);
        if (pos == std::string::npos) return items;

        pos += search.size();
        // Skip to opening bracket
        while (pos < json.size() && json[pos] != '[') pos++;
        if (pos >= json.size()) return items;
        pos++; // skip '['

        // Extract individual objects within the array
        int depth = 0;
        std::string current;
        while (pos < json.size()) {
            char c = json[pos];
            if (c == '{') {
                depth++;
                current += c;
            } else if (c == '}') {
                depth--;
                current += c;
                if (depth == 0) {
                    items.push_back(current);
                    current.clear();
                }
            } else if (c == ']' && depth == 0) {
                break;
            } else if (depth > 0) {
                current += c;
            }
            pos++;
        }
        return items;
    }

    // ─── API Parsers ────────────────────────────────────────────────────────────

    // Parse DuckDuckGo Instant Answer API response
    std::vector<MKSearchResult> parseDuckDuckGo(const std::string& response, const std::string& query) {
        std::vector<MKSearchResult> results;

        // Extract the abstract (main instant answer)
        std::string abstract = jsonGetString(response, "Abstract");
        std::string abstractURL = jsonGetString(response, "AbstractURL");
        std::string abstractSource = jsonGetString(response, "AbstractSource");

        if (!abstract.empty()) {
            MKSearchResult r;
            r.title = abstractSource.empty() ? "DuckDuckGo Abstract" : abstractSource;
            r.snippet = abstract;
            r.url = abstractURL;
            r.source = "duckduckgo";
            r.relevanceScore = 0.9f;
            results.push_back(r);
        }

        // Extract related topics
        auto relatedTopics = jsonGetArray(response, "RelatedTopics");
        for (const auto& topic : relatedTopics) {
            std::string text = jsonGetString(topic, "Text");
            std::string firstURL = jsonGetString(topic, "FirstURL");
            if (!text.empty()) {
                MKSearchResult r;
                r.title = text.substr(0, std::min((size_t)60, text.find(' ', 50)));
                r.snippet = text;
                r.url = firstURL;
                r.source = "duckduckgo";
                r.relevanceScore = 0.6f;
                results.push_back(r);
            }
            if (results.size() >= 5) break;
        }

        // Extract definition if available
        std::string definition = jsonGetString(response, "Definition");
        std::string definitionURL = jsonGetString(response, "DefinitionURL");
        if (!definition.empty()) {
            MKSearchResult r;
            r.title = "Definition: " + query;
            r.snippet = definition;
            r.url = definitionURL;
            r.source = "duckduckgo";
            r.relevanceScore = 0.85f;
            results.push_back(r);
        }

        return results;
    }

    // Parse Wikipedia REST API page summary response
    std::vector<MKSearchResult> parseWikipedia(const std::string& response) {
        std::vector<MKSearchResult> results;

        std::string title = jsonGetString(response, "title");
        std::string extract = jsonGetString(response, "extract");
        std::string description = jsonGetString(response, "description");
        std::string contentUrl = jsonGetString(response, "content_urls");

        // Try to get the desktop URL from nested structure
        std::string pageUrl;
        size_t desktopPos = response.find("\"desktop\"");
        if (desktopPos != std::string::npos) {
            size_t pagePos = response.find("\"page\"", desktopPos);
            if (pagePos != std::string::npos) {
                pageUrl = jsonGetString(response.substr(pagePos), "page");
            }
        }

        if (!extract.empty()) {
            MKSearchResult r;
            r.title = title.empty() ? "Wikipedia" : title;
            r.snippet = extract;
            r.url = pageUrl.empty() ? ("https://en.wikipedia.org/wiki/" + title) : pageUrl;
            r.source = "wikipedia";
            r.relevanceScore = 0.95f; // Wikipedia summaries are high quality
            results.push_back(r);
        }

        if (!description.empty() && description != extract) {
            MKSearchResult r;
            r.title = title + " (description)";
            r.snippet = description;
            r.url = pageUrl.empty() ? ("https://en.wikipedia.org/wiki/" + title) : pageUrl;
            r.source = "wikipedia";
            r.relevanceScore = 0.7f;
            results.push_back(r);
        }

        return results;
    }

public:
    MKSearchEngine() : totalSearches(0), successfulSearches(0) {
        std::cout << "[SEARCH] Multi-source internet search engine initialized.\n";
    }

    // Search a single source (DuckDuckGo by default)
    std::vector<MKSearchResult> search(const std::string& query) {
        totalSearches++;
        std::string encoded = urlEncode(query);

        // Query DuckDuckGo Instant Answer API
        std::string url = "https://api.duckduckgo.com/?q=" + encoded + "&format=json&no_html=1";
        std::string response = http.get(url);

        std::vector<MKSearchResult> results;
        if (!response.empty()) {
            results = parseDuckDuckGo(response, query);
            if (!results.empty()) successfulSearches++;
        }

        std::cout << "[SEARCH] Query: \"" << query << "\" -> " << results.size() << " results from DuckDuckGo\n";
        return results;
    }

    // Search multiple sources and aggregate results
    std::vector<MKSearchResult> searchMultiple(const std::string& query) {
        totalSearches++;
        std::string encoded = urlEncode(query);
        std::vector<MKSearchResult> allResults;

        // Source 1: DuckDuckGo
        std::string ddgUrl = "https://api.duckduckgo.com/?q=" + encoded + "&format=json&no_html=1";
        std::string ddgResponse = http.get(ddgUrl);
        if (!ddgResponse.empty()) {
            auto ddgResults = parseDuckDuckGo(ddgResponse, query);
            allResults.insert(allResults.end(), ddgResults.begin(), ddgResults.end());
        }

        // Source 2: Wikipedia
        // Replace spaces with underscores for Wikipedia article lookup
        std::string wikiQuery = query;
        std::replace(wikiQuery.begin(), wikiQuery.end(), ' ', '_');
        std::string wikiUrl = "https://en.wikipedia.org/api/rest_v1/page/summary/" + wikiQuery;
        std::string wikiResponse = http.get(wikiUrl);
        if (!wikiResponse.empty()) {
            auto wikiResults = parseWikipedia(wikiResponse);
            allResults.insert(allResults.end(), wikiResults.begin(), wikiResults.end());
        }

        // Sort by relevance score (highest first)
        std::sort(allResults.begin(), allResults.end(),
                  [](const MKSearchResult& a, const MKSearchResult& b) {
                      return a.relevanceScore > b.relevanceScore;
                  });

        // Deduplicate by URL
        std::vector<MKSearchResult> deduped;
        std::vector<std::string> seenUrls;
        for (const auto& r : allResults) {
            bool seen = false;
            for (const auto& u : seenUrls) {
                if (u == r.url) { seen = true; break; }
            }
            if (!seen) {
                deduped.push_back(r);
                if (!r.url.empty()) seenUrls.push_back(r.url);
            }
        }

        if (!deduped.empty()) successfulSearches++;

        std::cout << "[SEARCH] Multi-source query: \"" << query << "\" -> "
                  << deduped.size() << " unique results from " << 2 << " sources\n";
        return deduped;
    }

    // Get search statistics
    int getTotalSearches() const { return totalSearches; }
    int getSuccessfulSearches() const { return successfulSearches; }

    void printStats() const {
        std::cout << "[SEARCH STATS] Total: " << totalSearches
                  << " | Successful: " << successfulSearches << "\n";
    }
};

#endif // MK_SEARCH_ENGINE_CPP
