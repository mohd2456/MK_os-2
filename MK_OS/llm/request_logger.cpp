// ============================================================
// MK OS - LLM Request Logger
// Logs all LLM API calls with provider, latency, token usage,
// and success/failure. Writes JSON lines to llm_requests.log.
// ============================================================
#ifndef MK_REQUEST_LOGGER_CPP
#define MK_REQUEST_LOGGER_CPP

#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <ctime>
#include <chrono>
#include <mutex>
#include <algorithm>

struct MKRequestLogEntry {
    std::string provider;
    std::string input_summary;
    int tokens_used;
    float latency_ms;
    bool success;
    std::time_t timestamp;
};

class MKRequestLogger {
private:
    std::string logFilePath;
    std::vector<MKRequestLogEntry> todayEntries;
    mutable std::mutex logMutex;

    // Get today's date string (YYYY-MM-DD)
    std::string getTodayDate() const {
        std::time_t now = std::time(nullptr);
        struct tm* lt = std::localtime(&now);
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d",
                      lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday);
        return std::string(buf);
    }

    // Check if a timestamp is from today
    bool isToday(std::time_t ts) const {
        std::time_t now = std::time(nullptr);
        struct tm* nowTm = std::localtime(&now);
        int todayYear = nowTm->tm_year;
        int todayMon = nowTm->tm_mon;
        int todayDay = nowTm->tm_mday;

        struct tm* tsTm = std::localtime(&ts);
        return (tsTm->tm_year == todayYear &&
                tsTm->tm_mon == todayMon &&
                tsTm->tm_mday == todayDay);
    }

    // Escape a string for JSON output
    std::string jsonEscape(const std::string& s) const {
        std::string result;
        result.reserve(s.size());
        for (char c : s) {
            switch (c) {
                case '"': result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default: result += c; break;
            }
        }
        return result;
    }

public:
    MKRequestLogger(const std::string& logPath = "llm_requests.log")
        : logFilePath(logPath) {}

    // Log a single LLM request
    void logRequest(const std::string& provider,
                    const std::string& input_summary,
                    int tokens_used,
                    float latency_ms,
                    bool success) {
        std::lock_guard<std::mutex> lock(logMutex);

        MKRequestLogEntry entry;
        entry.provider = provider;
        entry.input_summary = input_summary.substr(0, 80); // Truncate to 80 chars
        entry.tokens_used = tokens_used;
        entry.latency_ms = latency_ms;
        entry.success = success;
        entry.timestamp = std::time(nullptr);

        // Keep in memory for today's stats
        todayEntries.push_back(entry);

        // Append JSON line to log file
        std::ofstream logFile(logFilePath, std::ios::app);
        if (logFile.is_open()) {
            logFile << "{\"timestamp\":" << entry.timestamp
                    << ",\"date\":\"" << getTodayDate() << "\""
                    << ",\"provider\":\"" << jsonEscape(entry.provider) << "\""
                    << ",\"input_summary\":\"" << jsonEscape(entry.input_summary) << "\""
                    << ",\"tokens_used\":" << entry.tokens_used
                    << ",\"latency_ms\":" << (int)entry.latency_ms
                    << ",\"success\":" << (entry.success ? "true" : "false")
                    << "}\n";
            logFile.close();
        }
    }

    // Get a string summary of today's requests
    std::string getStats() const {
        std::lock_guard<std::mutex> lock(logMutex);

        // Filter to today only
        std::vector<const MKRequestLogEntry*> todayOnly;
        for (const auto& e : todayEntries) {
            if (isToday(e.timestamp)) {
                todayOnly.push_back(&e);
            }
        }

        if (todayOnly.empty()) {
            return "No LLM requests logged today.";
        }

        int totalCalls = (int)todayOnly.size();
        int successCount = 0;
        float totalLatency = 0.0f;
        int totalTokens = 0;
        std::map<std::string, int> providerCounts;
        std::map<std::string, int> providerTokens;

        for (const auto* e : todayOnly) {
            if (e->success) successCount++;
            totalLatency += e->latency_ms;
            totalTokens += e->tokens_used;
            providerCounts[e->provider]++;
            providerTokens[e->provider] += e->tokens_used;
        }

        float avgLatency = totalLatency / (float)totalCalls;
        float successRate = (float)successCount / (float)totalCalls * 100.0f;

        std::ostringstream ss;
        ss << "<b>LLM Request Stats (Today)</b>\n\n";
        ss << "<code>Total calls:   " << totalCalls << "</code>\n";
        ss << "<code>Success rate:  " << (int)successRate << "%</code>\n";
        ss << "<code>Avg latency:   " << (int)avgLatency << " ms</code>\n";
        ss << "<code>Total tokens:  " << totalTokens << "</code>\n\n";

        ss << "<b>Per Provider:</b>\n";
        for (const auto& kv : providerCounts) {
            ss << "<code>  " << kv.first << ": " << kv.second << " calls, "
               << providerTokens[kv.first] << " tokens</code>\n";
        }

        return ss.str();
    }

    // Get daily token usage for a specific provider (for quota tracking)
    int getDailyUsage(const std::string& provider) const {
        std::lock_guard<std::mutex> lock(logMutex);

        int totalTokens = 0;
        for (const auto& e : todayEntries) {
            if (e.provider == provider && isToday(e.timestamp)) {
                totalTokens += e.tokens_used;
            }
        }
        return totalTokens;
    }

    // Get total number of entries logged today
    int getTodayCount() const {
        std::lock_guard<std::mutex> lock(logMutex);
        int count = 0;
        for (const auto& e : todayEntries) {
            if (isToday(e.timestamp)) count++;
        }
        return count;
    }

    // Get total requests logged (all time, in-memory)
    int getTotalCount() const {
        std::lock_guard<std::mutex> lock(logMutex);
        return (int)todayEntries.size();
    }

    // Get the log file path
    std::string getLogPath() const {
        return logFilePath;
    }

    // Clear in-memory entries (for testing)
    void clear() {
        std::lock_guard<std::mutex> lock(logMutex);
        todayEntries.clear();
    }
};

#endif // MK_REQUEST_LOGGER_CPP
