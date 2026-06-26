#ifndef MK_LOGGER_CPP
#define MK_LOGGER_CPP

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <ctime>
#include <mutex>
#include <sstream>

// ===================================================================================
// MK STRUCTURED LOGGING ENGINE
// Provides leveled, timestamped, structured logs with file persistence.
// Designed for zero-overhead when level is filtered — no string formatting occurs
// unless the message will actually be emitted.
// ===================================================================================

enum class MKLogLevel { 
    TRACE  = 0,   // Ultra-verbose internal debugging
    DEBUG  = 1,   // Developer-level diagnostic output
    INFO   = 2,   // Standard operational events
    WARN   = 3,   // Non-critical anomalies
    ERROR  = 4,   // Recoverable failures
    FATAL  = 5    // Unrecoverable — system will halt
};

struct MKLogEntry {
    std::time_t timestamp;
    MKLogLevel level;
    std::string subsystem;
    std::string message;
};

class MKLogger {
private:
    std::vector<MKLogEntry> logBuffer;
    std::ofstream logFile;
    MKLogLevel minLevel;
    std::mutex logMutex;
    size_t maxBufferSize;
    bool consoleOutput;
    bool fileOutput;
    std::string logFilePath;

    std::string levelToString(MKLogLevel lvl) const {
        switch (lvl) {
            case MKLogLevel::TRACE: return "TRACE";
            case MKLogLevel::DEBUG: return "DEBUG";
            case MKLogLevel::INFO:  return "INFO ";
            case MKLogLevel::WARN:  return "WARN ";
            case MKLogLevel::ERROR: return "ERROR";
            case MKLogLevel::FATAL: return "FATAL";
        }
        return "?????";
    }

    std::string formatTimestamp(std::time_t t) const {
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
        return std::string(buf);
    }

    std::string formatEntry(const MKLogEntry& entry) const {
        std::stringstream ss;
        ss << "[" << formatTimestamp(entry.timestamp) << "] "
           << "[" << levelToString(entry.level) << "] "
           << "[" << entry.subsystem << "] "
           << entry.message;
        return ss.str();
    }

    void flushToFile() {
        if (!fileOutput || !logFile.is_open()) return;
        for (const auto& entry : logBuffer) {
            logFile << formatEntry(entry) << "\n";
        }
        logFile.flush();
        logBuffer.clear();
    }

public:
    MKLogger() : minLevel(MKLogLevel::INFO), maxBufferSize(256),
                 consoleOutput(true), fileOutput(false), logFilePath("mk_system.log") {
        std::cout << "[LOGGER] Structured telemetry engine initialized.\n";
    }

    ~MKLogger() {
        if (logFile.is_open()) {
            flushToFile();
            logFile.close();
        }
    }

    // Enable file logging
    void enableFileOutput(const std::string& path = "mk_system.log") {
        logFilePath = path;
        logFile.open(logFilePath, std::ios::app);
        if (logFile.is_open()) {
            fileOutput = true;
            log("Logger file output enabled at: " + logFilePath);
        }
    }

    // Set minimum log level threshold
    void setLevel(MKLogLevel lvl) { minLevel = lvl; }
    void setConsoleOutput(bool enabled) { consoleOutput = enabled; }

    // Primary log function with level and subsystem tags
    void log(const std::string& message, MKLogLevel level = MKLogLevel::INFO, 
             const std::string& subsystem = "SYSTEM") {
        
        // Skip if below threshold — zero overhead for filtered messages
        if (level < minLevel) return;

        std::lock_guard<std::mutex> lock(logMutex);

        MKLogEntry entry;
        entry.timestamp = std::time(nullptr);
        entry.level = level;
        entry.subsystem = subsystem;
        entry.message = message;

        // Console output
        if (consoleOutput) {
            std::cout << formatEntry(entry) << "\n";
        }

        // Buffer for file persistence
        logBuffer.push_back(entry);

        // Auto-flush when buffer fills
        if (logBuffer.size() >= maxBufferSize) {
            flushToFile();
        }

        // Fatal messages trigger immediate flush
        if (level == MKLogLevel::FATAL) {
            flushToFile();
        }
    }

    // Convenience wrappers
    void trace(const std::string& msg, const std::string& sub = "SYSTEM") { log(msg, MKLogLevel::TRACE, sub); }
    void debug(const std::string& msg, const std::string& sub = "SYSTEM") { log(msg, MKLogLevel::DEBUG, sub); }
    void info(const std::string& msg, const std::string& sub = "SYSTEM")  { log(msg, MKLogLevel::INFO, sub); }
    void warn(const std::string& msg, const std::string& sub = "SYSTEM")  { log(msg, MKLogLevel::WARN, sub); }
    void error(const std::string& msg, const std::string& sub = "SYSTEM") { log(msg, MKLogLevel::ERROR, sub); }
    void fatal(const std::string& msg, const std::string& sub = "SYSTEM") { log(msg, MKLogLevel::FATAL, sub); }

    // Performance metrics logging
    void metric(const std::string& name, double value, const std::string& unit = "") {
        std::stringstream ss;
        ss << "METRIC[" << name << "] = " << value;
        if (!unit.empty()) ss << " " << unit;
        log(ss.str(), MKLogLevel::INFO, "METRICS");
    }

    // Flush all buffered entries to disk
    void flush() {
        std::lock_guard<std::mutex> lock(logMutex);
        flushToFile();
    }

    // Get total logged entries count
    size_t entryCount() const { return logBuffer.size(); }
};

#endif // MK_LOGGER_CPP
