// ============================================================
// MK OS - LLM Knowledge Eater Tool
// Connects to an LLM API endpoint, sends structured prompts to
// extract factual knowledge about a topic, parses responses into
// MK's source|relation|target|weight format, and outputs .mk files.
//
// Compiles standalone:
//   clang++ -std=c++17 -O2 tools/llm_eater.cpp -lcurl -o mk_llm_eater
//
// Usage:
//   ./mk_llm_eater <topic> <output.mk> [api_url]
//
// Default API: http://localhost:11434/api/generate (Ollama)
// ============================================================

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <cstring>
#include <curl/curl.h>

// ===================================================================================
// MK LLM EATER - KNOWLEDGE EXTRACTION FROM LANGUAGE MODELS
// Interrogates an LLM about a topic using structured prompts designed to extract
// factual triples. Parses freeform LLM output into source|relation|target|weight
// format for ingestion into MK's knowledge graph.
// ===================================================================================

// ============================================================
// HTTP Helper (standalone, no dependency on network/http.cpp)
// ============================================================

static size_t LLMWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    if (!userp) return 0;
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string httpPost(const std::string& url, const std::string& body) {
    CURL* curl = curl_easy_init();
    std::string response;

    if (curl) {
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, LLMWriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "  [HTTP ERROR] " << curl_easy_strerror(res) << "\n";
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    return response;
}

std::string httpGet(const std::string& url) {
    CURL* curl = curl_easy_init();
    std::string response;

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, LLMWriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "  [HTTP ERROR] " << curl_easy_strerror(res) << "\n";
        }

        curl_easy_cleanup(curl);
    }
    return response;
}

// ============================================================
// Utility Functions
// ============================================================

std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    auto end = s.find_last_not_of(" \t\r\n");
    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

// Simple JSON string value extractor
std::string jsonExtract(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";

    pos += search.size();
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == ':')) pos++;

    if (pos >= json.size()) return "";

    if (json[pos] == '"') {
        pos++;
        std::string value;
        while (pos < json.size()) {
            if (json[pos] == '\\' && pos + 1 < json.size()) {
                pos++;
                if (json[pos] == '"') value += '"';
                else if (json[pos] == 'n') value += '\n';
                else if (json[pos] == 't') value += '\t';
                else if (json[pos] == '\\') value += '\\';
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

    // Non-string value
    std::string value;
    while (pos < json.size() && json[pos] != ',' && json[pos] != '}') {
        value += json[pos];
        pos++;
    }
    return trim(value);
}

// Escape a string for JSON
std::string jsonEscape(const std::string& s) {
    std::string escaped;
    for (char c : s) {
        switch (c) {
            case '"': escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default: escaped += c;
        }
    }
    return escaped;
}

// ============================================================
// Fact Structure
// ============================================================

struct LLMFact {
    std::string source;
    std::string relation;
    std::string target;
    float weight;

    std::string toMK() const {
        std::ostringstream oss;
        oss << source << "|" << relation << "|" << target << "|";
        oss << std::to_string(weight).substr(0, 4);
        return oss.str();
    }
};

// ============================================================
// MKLLMEater Class
// ============================================================

class MKLLMEater {
private:
    std::string apiUrl;
    std::string model;
    int totalFacts;
    int totalQueries;

    // Build the extraction prompt for a topic
    std::string buildPrompt(const std::string& topic) {
        return "You are a knowledge extraction system. Extract factual relationships about \"" +
               topic + "\".\n\n"
               "Output ONLY lines in this exact format (one per line):\n"
               "subject|relation|object\n\n"
               "Use these relation types: is_a, has, can, uses, contains, part_of, "
               "created_by, located_in, related_to, defined_as, causes, requires, "
               "produces, invented_by, discovered_by, founded_in, belongs_to\n\n"
               "Extract 15-25 factual relationships. Be precise and factual. "
               "No explanations, no extra text, just the pipe-delimited triples.\n\n"
               "Topic: " + topic;
    }

    // Build JSON request body for Ollama-compatible API
    std::string buildRequestBody(const std::string& prompt) {
        return "{\"model\":\"" + jsonEscape(model) + "\","
               "\"prompt\":\"" + jsonEscape(prompt) + "\","
               "\"stream\":false,"
               "\"options\":{\"temperature\":0.3,\"num_predict\":1024}}";
    }

    // Build JSON request body for OpenAI-compatible API
    std::string buildOpenAIRequestBody(const std::string& prompt) {
        return "{\"model\":\"" + jsonEscape(model) + "\","
               "\"messages\":[{\"role\":\"user\",\"content\":\"" + jsonEscape(prompt) + "\"}],"
               "\"temperature\":0.3,\"max_tokens\":1024}";
    }

    // Extract the response text from various API response formats
    std::string extractResponseText(const std::string& response) {
        // Try Ollama format first: {"response": "..."}
        std::string text = jsonExtract(response, "response");
        if (!text.empty()) return text;

        // Try OpenAI format: look for "content" field
        text = jsonExtract(response, "content");
        if (!text.empty()) return text;

        // Try to find "text" field
        text = jsonExtract(response, "text");
        if (!text.empty()) return text;

        // Fallback: return raw response if it looks like plain text
        if (!response.empty() && response[0] != '{') return response;

        return "";
    }

public:
    MKLLMEater() : apiUrl("http://localhost:11434/api/generate"),
                   model("llama3"), totalFacts(0), totalQueries(0) {
        std::cout << "[LLM_EATER] LLM knowledge extraction tool initialized.\n";
    }

    void setApiUrl(const std::string& url) { apiUrl = url; }
    void setModel(const std::string& m) { model = m; }

    // Interrogate the LLM about a topic
    std::string interrogate(const std::string& topic) {
        totalQueries++;
        std::string prompt = buildPrompt(topic);
        std::string body = buildRequestBody(prompt);

        std::cout << "  Querying LLM about: \"" << topic << "\"...\n";
        std::string response = httpPost(apiUrl, body);

        if (response.empty()) {
            std::cerr << "  [ERROR] No response from LLM API at: " << apiUrl << "\n";
            return "";
        }

        std::string text = extractResponseText(response);
        if (text.empty()) {
            std::cerr << "  [ERROR] Could not parse LLM response.\n";
            return "";
        }

        std::cout << "  Got response (" << text.size() << " chars)\n";
        return text;
    }

    // Extract facts from LLM response text
    std::vector<LLMFact> extractFacts(const std::string& llmResponse) {
        std::vector<LLMFact> facts;
        std::istringstream stream(llmResponse);
        std::string line;

        while (std::getline(stream, line)) {
            line = trim(line);
            if (line.empty()) continue;

            // Skip lines that don't look like triples
            if (line[0] == '#' || line[0] == '-' || line[0] == '*') continue;
            if (line.find('|') == std::string::npos) continue;

            // Parse pipe-delimited format: source|relation|target
            std::istringstream ls(line);
            std::string source, relation, target;

            if (!std::getline(ls, source, '|')) continue;
            if (!std::getline(ls, relation, '|')) continue;
            if (!std::getline(ls, target, '|')) continue;

            source = trim(toLower(source));
            relation = trim(toLower(relation));
            target = trim(toLower(target));

            // Validate extracted fields
            if (source.empty() || relation.empty() || target.empty()) continue;
            if (source.size() > 100 || relation.size() > 50 || target.size() > 100) continue;

            // Check for an optional weight field
            float weight = 0.75f; // Default weight for LLM-extracted facts
            std::string weightStr;
            if (std::getline(ls, weightStr, '|')) {
                weightStr = trim(weightStr);
                try {
                    float w = std::stof(weightStr);
                    if (w >= 0.0f && w <= 1.0f) weight = w;
                } catch (...) {
                    // Keep default weight
                }
            }

            LLMFact fact;
            fact.source = source;
            fact.relation = relation;
            fact.target = target;
            fact.weight = weight;
            facts.push_back(fact);
        }

        totalFacts += (int)facts.size();
        return facts;
    }

    // Save extracted facts to a .mk file
    bool saveMKFormat(const std::vector<LLMFact>& facts, const std::string& outputPath,
                     const std::string& topic = "") {
        std::ofstream out(outputPath, std::ios::app);
        if (!out.is_open()) {
            std::cerr << "  [ERROR] Cannot open output file: " << outputPath << "\n";
            return false;
        }

        out << "\n# === LLM-Extracted Knowledge ===\n";
        out << "# Topic: " << topic << "\n";
        out << "# Date: " << __DATE__ << " " << __TIME__ << "\n";
        out << "# Facts: " << facts.size() << "\n";
        out << "# Source: LLM (" << model << " via " << apiUrl << ")\n\n";

        for (const auto& fact : facts) {
            out << fact.toMK() << "\n";
        }

        out.close();
        std::cout << "  Saved " << facts.size() << " facts to: " << outputPath << "\n";
        return true;
    }

    // Full pipeline: interrogate, extract, save
    int fullExtraction(const std::string& topic, const std::string& outputPath) {
        std::string response = interrogate(topic);
        if (response.empty()) return 0;

        auto facts = extractFacts(response);
        if (facts.empty()) {
            std::cout << "  [WARNING] No facts extracted from LLM response.\n";
            return 0;
        }

        saveMKFormat(facts, outputPath, topic);
        return (int)facts.size();
    }

    int getTotalFacts() const { return totalFacts; }
    int getTotalQueries() const { return totalQueries; }
};

// ============================================================
// Main
// ============================================================

void printUsage() {
    std::cout << R"(
  MK LLM Knowledge Eater
  =======================
  Extracts factual knowledge from an LLM API and converts
  it into MK's source|relation|target|weight format.

  Usage:
    ./mk_llm_eater <topic> <output.mk> [api_url] [model]

  Arguments:
    topic      - The subject to extract knowledge about
    output.mk  - Output file path (appends to existing)
    api_url    - LLM API endpoint (default: http://localhost:11434/api/generate)
    model      - Model name (default: llama3)

  Examples:
    ./mk_llm_eater "quantum computing" knowledge/quantum.mk
    ./mk_llm_eater "rust programming" facts.mk http://localhost:11434/api/generate mistral
    ./mk_llm_eater "neural networks" ai_facts.mk http://api.openai.com/v1/chat/completions gpt-4

  Output format:
    source|relation|target|weight

  The tool sends structured prompts to the LLM designed to
  extract factual relationships in triple format.
)" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printUsage();
        return 1;
    }

    std::string topic = argv[1];
    std::string outputFile = argv[2];
    std::string apiUrl = (argc > 3) ? argv[3] : "http://localhost:11434/api/generate";
    std::string model = (argc > 4) ? argv[4] : "llama3";

    std::cout << "\n  MK LLM Knowledge Eater\n";
    std::cout << "  ======================\n";
    std::cout << "  Topic:  " << topic << "\n";
    std::cout << "  Output: " << outputFile << "\n";
    std::cout << "  API:    " << apiUrl << "\n";
    std::cout << "  Model:  " << model << "\n\n";

    MKLLMEater eater;
    eater.setApiUrl(apiUrl);
    eater.setModel(model);

    int factsExtracted = eater.fullExtraction(topic, outputFile);

    std::cout << "\n  === Extraction Complete ===\n";
    std::cout << "  Facts extracted: " << factsExtracted << "\n";
    std::cout << "  Written to: " << outputFile << "\n\n";

    return (factsExtracted > 0) ? 0 : 1;
}
