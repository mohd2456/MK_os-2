#ifndef MK_API_CLIENT_CPP
#define MK_API_CLIENT_CPP

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstdio>
#include <chrono>

// ============================================================================
// MKAPIClient - REST API client for external service calls
// Simple HTTP client using system curl, with JSON parsing
// ============================================================================

enum class HttpMethod {
    GET,
    POST,
    PUT,
    DELETE_METHOD,
    PATCH
};

struct HttpResponse {
    int status_code;
    std::string body;
    std::map<std::string, std::string> headers;
    double response_time_ms;
    bool success;
    std::string error;
};

struct JsonValue {
    enum Type { STRING, NUMBER, BOOLEAN, ARRAY, OBJECT, NULL_TYPE };
    Type type;
    std::string string_value;
    double number_value;
    bool bool_value;
    std::vector<JsonValue> array_values;
    std::map<std::string, JsonValue> object_values;
};

struct APIEndpoint {
    std::string name;
    std::string base_url;
    std::string api_key;
    std::map<std::string, std::string> default_headers;
};

class MKAPIClient {
private:
    int timeout_seconds_;
    std::string user_agent_;
    std::map<std::string, APIEndpoint> endpoints_;
    HttpResponse last_response_;
    bool verbose_;

    std::string buildCurlCommand(const std::string& url, HttpMethod method,
                                  const std::string& body,
                                  const std::map<std::string, std::string>& headers,
                                  const std::string& output_file,
                                  const std::string& header_file) const {
        std::string cmd = "curl -s -L --max-time " + std::to_string(timeout_seconds_);
        
        // Method
        switch (method) {
            case HttpMethod::POST: cmd += " -X POST"; break;
            case HttpMethod::PUT: cmd += " -X PUT"; break;
            case HttpMethod::DELETE_METHOD: cmd += " -X DELETE"; break;
            case HttpMethod::PATCH: cmd += " -X PATCH"; break;
            default: break; // GET is default
        }

        // Headers
        cmd += " -A \"" + user_agent_ + "\"";
        for (const auto& h : headers) {
            cmd += " -H \"" + h.first + ": " + h.second + "\"";
        }

        // Body
        if (!body.empty()) {
            cmd += " -d '" + body + "'";
        }

        // Output files
        cmd += " -o " + output_file;
        cmd += " -D " + header_file;
        cmd += " -w \"%{http_code}\"";
        cmd += " \"" + url + "\"";

        return cmd;
    }

    HttpResponse executeRequest(const std::string& url, HttpMethod method,
                                const std::string& body,
                                const std::map<std::string, std::string>& headers) {
        HttpResponse response;
        response.success = false;
        response.status_code = 0;

        std::string output_file = "/tmp/mk_api_body_" + std::to_string(rand()) + ".tmp";
        std::string header_file = "/tmp/mk_api_hdrs_" + std::to_string(rand()) + ".tmp";

        std::string cmd = buildCurlCommand(url, method, body, headers, output_file, header_file);

        auto start = std::chrono::high_resolution_clock::now();

        // Execute and capture status code from stdout
        std::string status_cmd = cmd + " 2>/dev/null";
        FILE* pipe = popen(status_cmd.c_str(), "r");
        if (!pipe) {
            response.error = "Failed to execute curl command";
            return response;
        }

        char buffer[128];
        std::string status_str;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            status_str += buffer;
        }
        int ret = pclose(pipe);

        auto end = std::chrono::high_resolution_clock::now();
        response.response_time_ms = std::chrono::duration<double, std::milli>(end - start).count();

        if (ret != 0 && status_str.empty()) {
            response.error = "Request failed - network error or timeout";
            std::remove(output_file.c_str());
            std::remove(header_file.c_str());
            return response;
        }

        // Parse status code
        try {
            if (!status_str.empty()) {
                response.status_code = std::stoi(status_str.substr(status_str.length() - 3));
            }
        } catch (...) {
            response.status_code = 0;
        }

        // Read response body
        std::ifstream body_file(output_file);
        if (body_file.is_open()) {
            response.body = std::string((std::istreambuf_iterator<char>(body_file)),
                                       std::istreambuf_iterator<char>());
            body_file.close();
        }

        // Read response headers
        std::ifstream hdr_file(header_file);
        if (hdr_file.is_open()) {
            std::string line;
            while (std::getline(hdr_file, line)) {
                size_t colon = line.find(':');
                if (colon != std::string::npos) {
                    std::string key = line.substr(0, colon);
                    std::string value = line.substr(colon + 2);
                    if (!value.empty() && value.back() == '\r') value.pop_back();
                    response.headers[key] = value;
                }
            }
            hdr_file.close();
        }

        std::remove(output_file.c_str());
        std::remove(header_file.c_str());

        response.success = (response.status_code >= 200 && response.status_code < 300);
        last_response_ = response;

        if (verbose_) {
            std::cout << "[MKAPIClient] " << static_cast<int>(method) << " " << url
                      << " -> " << response.status_code << " ("
                      << response.response_time_ms << "ms)" << std::endl;
        }

        return response;
    }

    // Simple JSON string extraction (no external deps)
    std::string extractJsonString(const std::string& json, const std::string& key) const {
        std::string search = "\"" + key + "\"";
        size_t pos = json.find(search);
        if (pos == std::string::npos) return "";

        // Find the value
        pos = json.find(':', pos + search.length());
        if (pos == std::string::npos) return "";
        pos++;
        // Skip whitespace
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;

        if (pos >= json.size()) return "";

        if (json[pos] == '"') {
            // String value
            pos++;
            size_t end = pos;
            while (end < json.size() && json[end] != '"') {
                if (json[end] == '\\') end++; // Skip escaped chars
                end++;
            }
            return json.substr(pos, end - pos);
        } else {
            // Number, bool, null
            size_t end = json.find_first_of(",}]\n", pos);
            if (end == std::string::npos) end = json.size();
            std::string value = json.substr(pos, end - pos);
            // Trim
            while (!value.empty() && (value.back() == ' ' || value.back() == '\t'))
                value.pop_back();
            return value;
        }
    }

public:
    MKAPIClient() : timeout_seconds_(30), user_agent_("MK-OS/1.0 (API Client)"),
                    verbose_(false) {
        last_response_.success = false;
        initializeEndpoints();
    }

    void initializeEndpoints() {
        // Weather API
        APIEndpoint weather;
        weather.name = "weather";
        weather.base_url = "https://wttr.in";
        weather.default_headers = {{"Accept", "application/json"}};
        endpoints_["weather"] = weather;

        // Time API
        APIEndpoint time_api;
        time_api.name = "time";
        time_api.base_url = "https://worldtimeapi.org/api";
        time_api.default_headers = {{"Accept", "application/json"}};
        endpoints_["time"] = time_api;

        // JSONPlaceholder (for testing)
        APIEndpoint placeholder;
        placeholder.name = "placeholder";
        placeholder.base_url = "https://jsonplaceholder.typicode.com";
        placeholder.default_headers = {{"Content-Type", "application/json"}};
        endpoints_["placeholder"] = placeholder;
    }

    HttpResponse get(const std::string& url, 
                     const std::map<std::string, std::string>& headers = {}) {
        return executeRequest(url, HttpMethod::GET, "", headers);
    }

    HttpResponse post(const std::string& url, const std::string& body,
                      const std::map<std::string, std::string>& headers = {}) {
        std::map<std::string, std::string> h = headers;
        if (h.find("Content-Type") == h.end()) {
            h["Content-Type"] = "application/json";
        }
        return executeRequest(url, HttpMethod::POST, body, h);
    }

    std::map<std::string, std::string> parseJSON(const std::string& json_str) const {
        std::map<std::string, std::string> result;
        // Simple top-level key extraction
        size_t pos = 0;
        while (pos < json_str.size()) {
            size_t key_start = json_str.find('"', pos);
            if (key_start == std::string::npos) break;
            size_t key_end = json_str.find('"', key_start + 1);
            if (key_end == std::string::npos) break;

            std::string key = json_str.substr(key_start + 1, key_end - key_start - 1);

            // Find colon
            size_t colon = json_str.find(':', key_end);
            if (colon == std::string::npos) break;

            std::string value = extractJsonString(json_str, key);
            if (!value.empty()) {
                result[key] = value;
            }
            pos = key_end + 1;
        }
        return result;
    }

    // Built-in API methods
    std::string getWeather(const std::string& location) {
        std::string url = endpoints_["weather"].base_url + "/" + location + "?format=3";
        HttpResponse resp = get(url);
        return resp.success ? resp.body : "Weather unavailable: " + resp.error;
    }

    std::string getTime(const std::string& timezone = "Etc/UTC") {
        std::string url = endpoints_["time"].base_url + "/timezone/" + timezone;
        HttpResponse resp = get(url);
        if (resp.success) {
            return extractJsonString(resp.body, "datetime");
        }
        return "Time unavailable: " + resp.error;
    }

    void setTimeout(int seconds) { timeout_seconds_ = std::max(1, std::min(120, seconds)); }
    void setVerbose(bool v) { verbose_ = v; }
    void setUserAgent(const std::string& ua) { user_agent_ = ua; }

    HttpResponse getLastResponse() const { return last_response_; }

    void addEndpoint(const std::string& name, const APIEndpoint& endpoint) {
        endpoints_[name] = endpoint;
    }

    std::vector<std::string> listEndpoints() const {
        std::vector<std::string> names;
        for (const auto& kv : endpoints_) {
            names.push_back(kv.first);
        }
        return names;
    }

    bool hasEndpoint(const std::string& name) const {
        return endpoints_.find(name) != endpoints_.end();
    }
};

#endif // MK_API_CLIENT_CPP
