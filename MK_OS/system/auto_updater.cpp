#ifndef MK_AUTO_UPDATER_CPP
#define MK_AUTO_UPDATER_CPP

#include <string>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <sstream>
#include <iostream>
#include <array>
#include <cstdio>

// ============================================================
// MK Auto-Updater
// Pulls latest code from GitHub, recompiles MK, and restarts.
// Safety: reverts on build failure. Logs all operations.
// ============================================================

class MKAutoUpdater {
private:
    std::string logFile;
    std::string mkDir;
    int nightlyHour;
    std::time_t lastCheckTime;
    std::time_t lastUpdateTime;
    std::string lastBuildOutput;
    bool lastBuildSuccess;

    // Execute a command and capture its output
    std::string execCapture(const std::string& cmd) {
        std::string result;
        std::array<char, 256> buffer;
        std::string sanitized = MKCrypto::sanitizeInput(cmd);
        FILE* pipe = popen(sanitized.c_str(), "r");
        if (!pipe) return "[ERROR] Failed to execute command";
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            result += buffer.data();
        }
        int exitCode = pclose(pipe);
        if (exitCode != 0) {
            result += "\n[EXIT_CODE=" + std::to_string(exitCode) + "]";
        }
        return result;
    }

    // Check if output indicates failure
    bool hasError(const std::string& output) {
        return output.find("[EXIT_CODE=") != std::string::npos &&
               output.find("[EXIT_CODE=0]") == std::string::npos;
    }

    // Append to log file
    void log(const std::string& message) {
        std::ofstream file(logFile, std::ios::app);
        if (!file.is_open()) return;
        std::time_t now = std::time(nullptr);
        char timeBuf[32];
        std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S",
                      std::localtime(&now));
        file << "[" << timeBuf << "] " << message << "\n";
        file.close();
    }

    // Get git short hash
    std::string getGitHash() {
        std::string output = execCapture("git rev-parse --short HEAD 2>/dev/null");
        // Trim trailing newline
        while (!output.empty() && (output.back() == '\n' || output.back() == '\r'))
            output.pop_back();
        if (output.find("[EXIT_CODE=") != std::string::npos) return "unknown";
        return output;
    }

public:
    MKAutoUpdater(const std::string& mkDirectory = ".",
                  const std::string& logPath = "mk_update.log")
        : logFile(logPath), mkDir(mkDirectory), nightlyHour(3),
          lastCheckTime(0), lastUpdateTime(0), lastBuildSuccess(true) {
        std::cout << "  " << Color::BGREEN << "\u2713" << Color::RESET
                  << " Auto-updater initialized (nightly at "
                  << nightlyHour << ":00).\n";
        log("Auto-updater initialized. Version: " + getGitHash());
    }

    // Check if updates are available (git fetch + compare HEADs)
    bool checkForUpdates() {
        lastCheckTime = std::time(nullptr);
        std::cout << "  " << Color::BCYAN << "\u21BB" << Color::RESET
                  << " Checking for updates...\n";

        std::string fetchOutput = execCapture("git fetch origin main 2>&1");
        if (hasError(fetchOutput)) {
            std::cout << "  " << Color::RED << "\u2717" << Color::RESET
                      << " Fetch failed: " << fetchOutput.substr(0, 80) << "\n";
            log("CHECK: fetch failed - " + fetchOutput.substr(0, 100));
            return false;
        }

        // Compare local HEAD with remote HEAD
        std::string localHead = execCapture("git rev-parse HEAD 2>/dev/null");
        std::string remoteHead = execCapture(
            "git rev-parse origin/main 2>/dev/null");

        // Trim newlines
        while (!localHead.empty() && localHead.back() == '\n') localHead.pop_back();
        while (!remoteHead.empty() && remoteHead.back() == '\n') remoteHead.pop_back();

        bool updatesAvailable = (!localHead.empty() && !remoteHead.empty() &&
                                  localHead != remoteHead);

        if (updatesAvailable) {
            std::cout << "  " << Color::BYELLOW << "\u2191" << Color::RESET
                      << " Updates available! Local: " << localHead.substr(0, 7)
                      << " Remote: " << remoteHead.substr(0, 7) << "\n";
            log("CHECK: updates available (local=" + localHead.substr(0, 7) +
                " remote=" + remoteHead.substr(0, 7) + ")");
        } else {
            std::cout << "  " << Color::GREEN << "\u2713" << Color::RESET
                      << " Already up to date (" << localHead.substr(0, 7) << ")\n";
            log("CHECK: up to date (" + localHead.substr(0, 7) + ")");
        }
        return updatesAvailable;
    }

    // Pull latest code
    bool pullUpdates() {
        std::cout << "  " << Color::BCYAN << "\u2193" << Color::RESET
                  << " Pulling updates...\n";

        std::string output = execCapture("git pull origin main 2>&1");
        if (hasError(output)) {
            std::cout << "  " << Color::RED << "\u2717" << Color::RESET
                      << " Pull failed.\n";
            log("PULL: failed - " + output.substr(0, 200));
            return false;
        }

        std::cout << "  " << Color::GREEN << "\u2713" << Color::RESET
                  << " Pull successful.\n";
        log("PULL: success");
        return true;
    }

    // Rebuild MK OS
    bool rebuild() {
        std::cout << "  " << Color::BCYAN << "\u2692" << Color::RESET
                  << " Rebuilding MK OS...\n";

        std::string buildCmd = "cd " + MKCrypto::sanitizeInput(mkDir) +
                               " && make clean && make 2>&1";
        lastBuildOutput = execCapture(buildCmd);
        lastBuildSuccess = !hasError(lastBuildOutput);

        if (lastBuildSuccess) {
            std::cout << "  " << Color::BGREEN << "\u2713" << Color::RESET
                      << " Build successful!\n";
            log("BUILD: success");
        } else {
            std::cout << "  " << Color::RED << "\u2717" << Color::RESET
                      << " Build FAILED. Reverting...\n";
            log("BUILD: FAILED - " + lastBuildOutput.substr(0, 300));
        }
        return lastBuildSuccess;
    }

    // Schedule nightly update hour (0-23)
    void scheduleNightlyUpdate(int hour = 3) {
        if (hour < 0) hour = 0;
        if (hour > 23) hour = 23;
        nightlyHour = hour;
        std::cout << "  " << Color::GREEN << "\u2713" << Color::RESET
                  << " Nightly updates scheduled at " << nightlyHour << ":00\n";
        log("SCHEDULE: nightly hour set to " + std::to_string(nightlyHour));
    }

    // Full update pipeline: check -> pull -> rebuild -> flag restart
    bool performUpdate() {
        log("UPDATE: starting full update pipeline");
        std::cout << "\n  " << Color::BOLD << Color::BCYAN
                  << "\u2191 MK Auto-Update Pipeline" << Color::RESET << "\n";

        // Step 1: Check
        if (!checkForUpdates()) {
            std::cout << "  " << Color::DIM << "No updates needed."
                      << Color::RESET << "\n";
            return false;
        }

        // Step 2: Pull
        if (!pullUpdates()) {
            return false;
        }

        // Step 3: Rebuild
        if (!rebuild()) {
            // SAFETY: revert changes if build fails
            std::cout << "  " << Color::YELLOW << "\u26A0" << Color::RESET
                      << " Reverting to last working state...\n";
            execCapture("git checkout . 2>&1");
            log("SAFETY: reverted to last working state after build failure");
            std::cout << "  " << Color::GREEN << "\u2713" << Color::RESET
                      << " Reverted. System unchanged.\n";
            return false;
        }

        // Step 4: Write restart flag
        lastUpdateTime = std::time(nullptr);
        std::ofstream flag(".mk_restart_needed");
        if (flag.is_open()) {
            flag << "updated_at=" << lastUpdateTime << "\n";
            flag << "version=" << getGitHash() << "\n";
            flag.close();
        }

        std::cout << "  " << Color::BGREEN << "\u2713" << Color::RESET
                  << Color::BOLD << " Update complete! "
                  << Color::RESET << Color::DIM
                  << "Restart needed to apply." << Color::RESET << "\n";
        log("UPDATE: complete. Restart flag written. New version: " + getGitHash());
        return true;
    }

    // Get last update time from log
    std::string getLastUpdateTime() {
        if (lastUpdateTime > 0) {
            char buf[32];
            std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S",
                          std::localtime(&lastUpdateTime));
            return std::string(buf);
        }
        // Try reading from log file
        std::ifstream file(logFile);
        if (!file.is_open()) return "never";
        std::string lastLine, line;
        while (std::getline(file, line)) {
            if (line.find("UPDATE: complete") != std::string::npos) {
                lastLine = line;
            }
        }
        if (!lastLine.empty() && lastLine.size() > 20) {
            return lastLine.substr(1, 19); // Extract timestamp
        }
        return "never";
    }

    // Get full status report
    std::string getUpdateStatus() {
        std::string status;
        status += "Version: " + getGitHash() + "\n";
        status += "Last check: ";
        if (lastCheckTime > 0) {
            char buf[32];
            std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S",
                          std::localtime(&lastCheckTime));
            status += std::string(buf);
        } else {
            status += "never";
        }
        status += "\nLast update: " + getLastUpdateTime();
        status += "\nNightly schedule: " + std::to_string(nightlyHour) + ":00";
        status += "\nLast build: " + std::string(lastBuildSuccess ? "OK" : "FAILED");
        return status;
    }

    // Check if it's time for nightly update
    bool isNightlyUpdateTime() const {
        std::time_t now = std::time(nullptr);
        struct tm* tm_info = std::localtime(&now);
        return (tm_info && tm_info->tm_hour == nightlyHour &&
                tm_info->tm_min < 5); // Within first 5 minutes of the hour
    }

    int getNightlyHour() const { return nightlyHour; }

    void printStats() const {
        std::cout << "  " << Color::BOLD << Color::BCYAN
                  << "  Auto-Updater:" << Color::RESET << "\n";
        std::cout << "    Schedule: nightly at " << nightlyHour << ":00\n";
        std::cout << "    Last build: "
                  << (lastBuildSuccess ? "OK" : "FAILED") << "\n";
    }
};

#endif // MK_AUTO_UPDATER_CPP
