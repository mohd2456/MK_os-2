#ifndef MK_PAGE_SCRAPER_CPP
#define MK_PAGE_SCRAPER_CPP

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <chrono>

// ============================================================================
// MKPageScraper - Extracts content from web pages
// Simple HTML parser with tag stripping and content extraction
// ============================================================================

struct PageContent {
    std::string title;
    std::string body_text;
    std::string url;
    int word_count;
    std::vector<std::string> headings;
    std::vector<std::string> links;
    std::vector<std::string> paragraphs;
};

struct FetchResult {
    std::string html;
    int status_code;
    std::string content_type;
    bool success;
    std::string error;
    double fetch_time_ms;
};

class MKPageScraper {
private:
    int timeout_seconds_;
    std::string user_agent_;
    size_t max_content_size_;
    FetchResult last_fetch_;

    std::string runCurl(const std::string& url) {
        std::string temp_file = "/tmp/mk_scraper_" + std::to_string(rand()) + ".html";
        std::string header_file = "/tmp/mk_scraper_headers_" + std::to_string(rand()) + ".txt";

        std::string cmd = "curl -s -L --max-time " + std::to_string(timeout_seconds_)
                        + " -A \"" + user_agent_ + "\""
                        + " -D " + header_file
                        + " -o " + temp_file
                        + " \"" + url + "\" 2>/dev/null";

        auto start = std::chrono::high_resolution_clock::now();
        int ret = system(cmd.c_str());
        auto end = std::chrono::high_resolution_clock::now();

        last_fetch_.fetch_time_ms = std::chrono::duration<double, std::milli>(end - start).count();

        if (ret != 0) {
            last_fetch_.success = false;
            last_fetch_.error = "curl failed with exit code: " + std::to_string(ret);
            std::remove(temp_file.c_str());
            std::remove(header_file.c_str());
            return "";
        }

        // Read content
        std::ifstream file(temp_file);
        if (!file.is_open()) {
            last_fetch_.success = false;
            last_fetch_.error = "Could not read downloaded content";
            return "";
        }
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();

        // Read headers for status code
        std::ifstream hdr_file(header_file);
        if (hdr_file.is_open()) {
            std::string first_line;
            std::getline(hdr_file, first_line);
            size_t space_pos = first_line.find(' ');
            if (space_pos != std::string::npos) {
                std::string code_str = first_line.substr(space_pos + 1, 3);
                try { last_fetch_.status_code = std::stoi(code_str); } catch (...) {}
            }
            hdr_file.close();
        }

        std::remove(temp_file.c_str());
        std::remove(header_file.c_str());

        last_fetch_.success = true;
        last_fetch_.html = content;
        return content;
    }

    std::string stripAllTags(const std::string& html) const {
        std::string text;
        bool in_tag = false;
        bool in_script = false;
        bool in_style = false;
        std::string tag_name;

        for (size_t i = 0; i < html.size(); i++) {
            if (html[i] == '<') {
                in_tag = true;
                tag_name.clear();
                continue;
            }
            if (html[i] == '>') {
                in_tag = false;
                std::string lower_tag = tag_name;
                std::transform(lower_tag.begin(), lower_tag.end(), lower_tag.begin(), ::tolower);
                if (lower_tag.find("script") == 0) in_script = true;
                if (lower_tag.find("/script") == 0) in_script = false;
                if (lower_tag.find("style") == 0) in_style = true;
                if (lower_tag.find("/style") == 0) in_style = false;
                // Add space after block elements
                if (lower_tag.find("/p") == 0 || lower_tag.find("/div") == 0 ||
                    lower_tag.find("/h") == 0 || lower_tag.find("/li") == 0 ||
                    lower_tag == "br" || lower_tag == "br/") {
                    text += "\n";
                }
                continue;
            }
            if (in_tag) {
                tag_name += html[i];
            } else if (!in_script && !in_style) {
                text += html[i];
            }
        }
        return text;
    }

    std::string decodeEntities(const std::string& text) const {
        std::string result = text;
        std::vector<std::pair<std::string, std::string>> entities = {
            {"&amp;", "&"}, {"&lt;", "<"}, {"&gt;", ">"},
            {"&quot;", "\""}, {"&#39;", "'"}, {"&nbsp;", " "},
            {"&mdash;", "-"}, {"&ndash;", "-"}, {"&hellip;", "..."},
            {"&rsquo;", "'"}, {"&lsquo;", "'"}, {"&rdquo;", "\""},
            {"&ldquo;", "\""}, {"&copy;", "(c)"}, {"&reg;", "(R)"}
        };
        for (const auto& ent : entities) {
            size_t pos;
            while ((pos = result.find(ent.first)) != std::string::npos) {
                result.replace(pos, ent.first.length(), ent.second);
            }
        }
        return result;
    }

    std::string cleanWhitespace(const std::string& text) const {
        std::string result;
        bool last_was_space = false;
        for (char c : text) {
            if (c == '\n') {
                if (!last_was_space) result += '\n';
                last_was_space = true;
            } else if (c == ' ' || c == '\t' || c == '\r') {
                if (!last_was_space) result += ' ';
                last_was_space = true;
            } else {
                result += c;
                last_was_space = false;
            }
        }
        return result;
    }

    std::vector<std::string> extractByTagName(const std::string& html, 
                                               const std::string& tag) const {
        std::vector<std::string> results;
        std::string open_tag = "<" + tag;
        std::string close_tag = "</" + tag + ">";
        size_t pos = 0;

        while (pos < html.size()) {
            size_t start = html.find(open_tag, pos);
            if (start == std::string::npos) break;
            // Find end of opening tag
            size_t tag_end = html.find(">", start);
            if (tag_end == std::string::npos) break;
            // Find closing tag
            size_t close = html.find(close_tag, tag_end);
            if (close == std::string::npos) break;

            std::string content = html.substr(tag_end + 1, close - tag_end - 1);
            content = stripAllTags(content);
            content = decodeEntities(content);
            std::string trimmed = cleanWhitespace(content);
            size_t first = trimmed.find_first_not_of(" \n\t");
            if (first != std::string::npos) {
                size_t last = trimmed.find_last_not_of(" \n\t");
                trimmed = trimmed.substr(first, last - first + 1);
                if (!trimmed.empty()) results.push_back(trimmed);
            }
            pos = close + close_tag.length();
        }
        return results;
    }

public:
    MKPageScraper() : timeout_seconds_(15), 
                      user_agent_("MK-OS/1.0 (Page Scraper)"),
                      max_content_size_(1048576) { // 1MB
        last_fetch_.success = false;
        last_fetch_.status_code = 0;
    }

    FetchResult fetch(const std::string& url) {
        last_fetch_ = FetchResult();
        last_fetch_.success = false;

        std::string html = runCurl(url);
        if (html.empty() && !last_fetch_.success) {
            return last_fetch_;
        }

        if (html.size() > max_content_size_) {
            html = html.substr(0, max_content_size_);
        }

        last_fetch_.html = html;
        last_fetch_.success = true;
        return last_fetch_;
    }

    std::string extractText(const std::string& html) const {
        std::string stripped = stripAllTags(html);
        std::string decoded = decodeEntities(stripped);
        return cleanWhitespace(decoded);
    }

    std::vector<std::string> extractByTag(const std::string& html, const std::string& tag) const {
        return extractByTagName(html, tag);
    }

    std::string getTitle(const std::string& html) const {
        std::vector<std::string> titles = extractByTagName(html, "title");
        if (!titles.empty()) return titles[0];
        // Try h1 as fallback
        std::vector<std::string> h1s = extractByTagName(html, "h1");
        if (!h1s.empty()) return h1s[0];
        return "";
    }

    PageContent extractFullContent(const std::string& url) {
        PageContent content;
        content.url = url;

        FetchResult result = fetch(url);
        if (!result.success) return content;

        content.title = getTitle(result.html);
        content.body_text = extractText(result.html);

        // Extract headings
        for (int i = 1; i <= 6; i++) {
            std::string tag = "h" + std::to_string(i);
            auto headings = extractByTagName(result.html, tag);
            content.headings.insert(content.headings.end(), headings.begin(), headings.end());
        }

        // Extract paragraphs
        content.paragraphs = extractByTagName(result.html, "p");

        // Count words
        std::istringstream stream(content.body_text);
        std::string word;
        content.word_count = 0;
        while (stream >> word) content.word_count++;

        return content;
    }

    void setTimeout(int seconds) { timeout_seconds_ = std::max(1, std::min(60, seconds)); }
    void setUserAgent(const std::string& ua) { user_agent_ = ua; }
    void setMaxContentSize(size_t bytes) { max_content_size_ = bytes; }

    FetchResult getLastFetch() const { return last_fetch_; }
    bool wasSuccessful() const { return last_fetch_.success; }
};

#endif // MK_PAGE_SCRAPER_CPP
