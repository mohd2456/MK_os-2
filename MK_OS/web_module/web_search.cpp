#ifndef MK_WEB_SEARCH_CPP
#define MK_WEB_SEARCH_CPP

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <array>
#include <chrono>

// ============================================================================
// MKWebSearch - Internet search functionality
// Builds search URLs, fetches via HTTP, parses HTML for result snippets
// ============================================================================

struct SearchResult {
    std::string title;
    std::string url;
    std::string snippet;
    int rank;
};

struct SearchConfig {
    int num_results;
    int timeout_seconds;
    std::string user_agent;
    bool safe_search;
    std::string region;
};

enum class SearchEngine {
    DUCKDUCKGO,
    CUSTOM
};

class MKWebSearch {
private:
    SearchConfig config_;
    std::vector<SearchResult> last_results_;
    bool last_search_success_;
    std::string last_error_;

    std::string urlEncode(const std::string& str) const {
        std::string encoded;
        for (char c : str) {
            if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                encoded += c;
            } else if (c == ' ') {
                encoded += '+';
            } else {
                char hex[4];
                std::snprintf(hex, sizeof(hex), "%%%02X", static_cast<unsigned char>(c));
                encoded += hex;
            }
        }
        return encoded;
    }

    std::string buildDuckDuckGoURL(const std::string& query) const {
        return "https://html.duckduckgo.com/html/?q=" + urlEncode(query);
    }

    std::string fetchURL(const std::string& url) {
        std::string temp_file = "/tmp/mk_search_result.html";
        std::string cmd = "curl -s -L --max-time " + std::to_string(config_.timeout_seconds)
                        + " -A \"" + config_.user_agent + "\""
                        + " -o " + temp_file
                        + " \"" + url + "\" 2>&1";

        int ret = system(cmd.c_str());
        if (ret != 0) {
            last_error_ = "HTTP request failed with code: " + std::to_string(ret);
            return "";
        }

        std::ifstream file(temp_file);
        if (!file.is_open()) {
            last_error_ = "Could not read response file";
            return "";
        }

        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();
        std::remove(temp_file.c_str());
        return content;
    }

    std::string stripTags(const std::string& html) const {
        std::string text;
        bool in_tag = false;
        for (char c : html) {
            if (c == '<') { in_tag = true; continue; }
            if (c == '>') { in_tag = false; continue; }
            if (!in_tag) text += c;
        }
        return text;
    }

    std::string trimWhitespace(const std::string& str) const {
        size_t start = str.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return "";
        size_t end = str.find_last_not_of(" \t\n\r");
        return str.substr(start, end - start + 1);
    }

    std::string decodeHTMLEntities(const std::string& html) const {
        std::string decoded = html;
        std::vector<std::pair<std::string, std::string>> entities = {
            {"&amp;", "&"}, {"&lt;", "<"}, {"&gt;", ">"},
            {"&quot;", "\""}, {"&#39;", "'"}, {"&nbsp;", " "},
            {"&#x27;", "'"}, {"&#x2F;", "/"}
        };
        for (const auto& ent : entities) {
            size_t pos;
            while ((pos = decoded.find(ent.first)) != std::string::npos) {
                decoded.replace(pos, ent.first.length(), ent.second);
            }
        }
        return decoded;
    }

    std::string extractBetween(const std::string& html, const std::string& start_tag,
                               const std::string& end_tag, size_t& search_pos) const {
        size_t start = html.find(start_tag, search_pos);
        if (start == std::string::npos) return "";
        start += start_tag.length();
        size_t end = html.find(end_tag, start);
        if (end == std::string::npos) return "";
        search_pos = end + end_tag.length();
        return html.substr(start, end - start);
    }

    std::vector<SearchResult> parseDuckDuckGoHTML(const std::string& html) const {
        std::vector<SearchResult> results;
        size_t pos = 0;
        int rank = 1;

        // DuckDuckGo HTML results are in <div class="result"> blocks
        while (pos < html.length() && static_cast<int>(results.size()) < config_.num_results) {
            // Find result link
            size_t link_start = html.find("class=\"result__a\"", pos);
            if (link_start == std::string::npos) {
                // Try alternative format
                link_start = html.find("class=\"result__url\"", pos);
                if (link_start == std::string::npos) break;
            }

            SearchResult result;

            // Extract URL
            size_t href_pos = html.rfind("href=\"", link_start);
            if (href_pos != std::string::npos && (link_start - href_pos) < 200) {
                size_t href_end = html.find("\"", href_pos + 6);
                if (href_end != std::string::npos) {
                    result.url = html.substr(href_pos + 6, href_end - href_pos - 6);
                }
            }

            // Extract title
            size_t title_start = html.find(">", link_start);
            if (title_start != std::string::npos) {
                size_t title_end = html.find("</a>", title_start);
                if (title_end != std::string::npos) {
                    result.title = stripTags(html.substr(title_start + 1, title_end - title_start - 1));
                    result.title = decodeHTMLEntities(trimWhitespace(result.title));
                }
            }

            // Extract snippet
            size_t snippet_start = html.find("class=\"result__snippet\"", link_start);
            if (snippet_start != std::string::npos) {
                size_t snippet_tag_end = html.find(">", snippet_start);
                if (snippet_tag_end != std::string::npos) {
                    size_t snippet_end = html.find("</", snippet_tag_end);
                    if (snippet_end != std::string::npos) {
                        result.snippet = stripTags(html.substr(snippet_tag_end + 1, 
                                                              snippet_end - snippet_tag_end - 1));
                        result.snippet = decodeHTMLEntities(trimWhitespace(result.snippet));
                    }
                }
            }

            pos = link_start + 1;

            if (!result.title.empty() || !result.url.empty()) {
                result.rank = rank++;
                results.push_back(result);
            }
        }
        return results;
    }

public:
    MKWebSearch() : last_search_success_(false) {
        config_.num_results = 5;
        config_.timeout_seconds = 10;
        config_.user_agent = "MK-OS/1.0 (Search Bot)";
        config_.safe_search = true;
        config_.region = "us-en";
    }

    explicit MKWebSearch(const SearchConfig& config) 
        : config_(config), last_search_success_(false) {}

    std::vector<SearchResult> search(const std::string& query, int num_results = -1) {
        if (num_results > 0) config_.num_results = num_results;

        last_results_.clear();
        last_search_success_ = false;
        last_error_.clear();

        std::string url = buildDuckDuckGoURL(query);
        std::string html = fetchURL(url);

        if (html.empty()) {
            if (last_error_.empty()) {
                last_error_ = "No response received - check internet connection";
            }
            return last_results_;
        }

        last_results_ = parseDuckDuckGoHTML(html);
        last_search_success_ = !last_results_.empty();

        if (last_results_.empty()) {
            last_error_ = "No results found for: " + query;
        }

        return last_results_;
    }

    std::vector<SearchResult> searchDuckDuckGo(const std::string& query) {
        return search(query);
    }

    std::vector<SearchResult> parseResults(const std::string& html) {
        return parseDuckDuckGoHTML(html);
    }

    std::vector<SearchResult> getLastResults() const { return last_results_; }
    bool wasSuccessful() const { return last_search_success_; }
    std::string getLastError() const { return last_error_; }

    void setNumResults(int n) { config_.num_results = std::max(1, std::min(20, n)); }
    void setTimeout(int seconds) { config_.timeout_seconds = std::max(1, std::min(60, seconds)); }
    void setUserAgent(const std::string& ua) { config_.user_agent = ua; }

    SearchConfig getConfig() const { return config_; }
};

#endif // MK_WEB_SEARCH_CPP
