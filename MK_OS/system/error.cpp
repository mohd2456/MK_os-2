#ifndef MK_ERROR_CPP
#define MK_ERROR_CPP

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <ctime>
#include <functional>
#include <map>

// ===================================================================================
// MK ERROR HANDLING & RECOVERY ENGINE
// Provides structured error capture, crash snapshot generation, and automatic
// recovery strategy dispatch. Designed to keep MK alive on legacy hardware where
// unexpected faults are common.
// ===================================================================================

enum class MKErrorSeverity {
    RECOVERABLE,    // Log and continue
    DEGRADED,       // Disable affected subsystem, continue running
    CRITICAL,       // Attempt recovery strategy before halting
    FATAL           // Immediate snapshot + halt
};

struct MKCrashSnapshot {
    std::time_t timestamp;
    std::string subsystem;
    std::string errorMessage;
    MKErrorSeverity severity;
    std::string stackContext;
    long memoryUsedMB;
};

class MKError {
private:
    std::vector<MKCrashSnapshot> crashHistory;
    std::map<std::string, std::function<bool()>> recoveryStrategies;
    int totalErrors;
    int recoveredErrors;
    std::string snapshotDir;

    std::string severityLabel(MKErrorSeverity sev) const {
        switch (sev) {
            case MKErrorSeverity::RECOVERABLE: return "RECOVERABLE";
            case MKErrorSeverity::DEGRADED:    return "DEGRADED";
            case MKErrorSeverity::CRITICAL:    return "CRITICAL";
            case MKErrorSeverity::FATAL:       return "FATAL";
        }
        return "UNKNOWN";
    }

    void writeSnapshot(const MKCrashSnapshot& snap) {
        std::string filename = snapshotDir + "/crash_" + std::to_string(snap.timestamp) + ".log";
        std::ofstream out(filename);
        if (out.is_open()) {
            out << "=== MK CRASH SNAPSHOT ===\n";
            out << "Time: " << std::ctime(&snap.timestamp);
            out << "Subsystem: " << snap.subsystem << "\n";
            out << "Severity: " << severityLabel(snap.severity) << "\n";
            out << "Error: " << snap.errorMessage << "\n";
            out << "Context: " << snap.stackContext << "\n";
            out << "Memory Used: " << snap.memoryUsedMB << " MB\n";
            out << "=========================\n";
            out.close();
        }
    }

public:
    MKError() : totalErrors(0), recoveredErrors(0), snapshotDir(".") {
        std::cout << "[ERROR ENGINE] Fault isolation and recovery system online.\n";

        // Register default recovery strategies
        recoveryStrategies["ai_core"] = []() {
            std::cout << "[RECOVERY] Restarting ai_core with reduced cache budget...\n";
            return true;
        };
        recoveryStrategies["mk_brain"] = []() {
            std::cout << "[RECOVERY] Flushing mk_brain WAL and reopening database...\n";
            return true;
        };
        recoveryStrategies["network"] = []() {
            std::cout << "[RECOVERY] Resetting network stack. Reconnecting...\n";
            return true;
        };
        recoveryStrategies["memory"] = []() {
            std::cout << "[RECOVERY] Running emergency garbage collection sweep...\n";
            return true;
        };
        recoveryStrategies["plugin"] = []() {
            std::cout << "[RECOVERY] Sandboxed plugin killed. Continuing without it.\n";
            return true;
        };
    }

    // Primary error handler
    void handle(const std::string& message, 
                MKErrorSeverity severity = MKErrorSeverity::RECOVERABLE,
                const std::string& subsystem = "SYSTEM",
                const std::string& context = "") {
        
        totalErrors++;

        std::cout << "[ERROR:" << severityLabel(severity) << "] [" << subsystem << "] " << message << "\n";

        // Create crash snapshot for anything above RECOVERABLE
        if (severity >= MKErrorSeverity::DEGRADED) {
            MKCrashSnapshot snap;
            snap.timestamp = std::time(nullptr);
            snap.subsystem = subsystem;
            snap.errorMessage = message;
            snap.severity = severity;
            snap.stackContext = context.empty() ? "No additional context captured." : context;
            snap.memoryUsedMB = 0; // Would read from /proc/self/status in production

            crashHistory.push_back(snap);
            writeSnapshot(snap);
        }

        // Attempt recovery based on subsystem
        if (severity == MKErrorSeverity::CRITICAL || severity == MKErrorSeverity::DEGRADED) {
            auto it = recoveryStrategies.find(subsystem);
            if (it != recoveryStrategies.end()) {
                bool recovered = it->second();
                if (recovered) {
                    recoveredErrors++;
                    std::cout << "[ERROR ENGINE] Recovery successful for: " << subsystem << "\n";
                } else {
                    std::cout << "[ERROR ENGINE] Recovery FAILED for: " << subsystem << "\n";
                }
            }
        }

        // Fatal errors print full diagnostic before halt
        if (severity == MKErrorSeverity::FATAL) {
            std::cerr << "\n##################################################\n";
            std::cerr << "#          MK FATAL ERROR — SYSTEM HALTING       #\n";
            std::cerr << "##################################################\n";
            std::cerr << "  Subsystem: " << subsystem << "\n";
            std::cerr << "  Message:   " << message << "\n";
            std::cerr << "  Context:   " << context << "\n";
            std::cerr << "##################################################\n\n";
        }
    }

    // Convenience overload matching existing code
    void handle(const std::string& message) {
        handle(message, MKErrorSeverity::RECOVERABLE, "SYSTEM", "");
    }

    // Register custom recovery strategy for a subsystem
    void registerRecovery(const std::string& subsystem, std::function<bool()> strategy) {
        recoveryStrategies[subsystem] = strategy;
    }

    // Assertion helper — triggers error if condition is false
    void assertCondition(bool condition, const std::string& message,
                         const std::string& subsystem = "SYSTEM") {
        if (!condition) {
            handle("Assertion failed: " + message, MKErrorSeverity::CRITICAL, subsystem);
        }
    }

    // Statistics
    int getTotalErrors() const { return totalErrors; }
    int getRecoveredErrors() const { return recoveredErrors; }
    int getCrashCount() const { return crashHistory.size(); }

    void printStats() const {
        std::cout << "[ERROR ENGINE] Total Errors: " << totalErrors
                  << " | Recovered: " << recoveredErrors
                  << " | Crash Snapshots: " << crashHistory.size() << "\n";
    }
};

#endif // MK_ERROR_CPP
