#ifndef MK_CODE_RUNNER_CPP
#define MK_CODE_RUNNER_CPP

#include <string>
#include <fstream>
#include <cstdio>
#include <array>
#include <chrono>
#include <iostream>

// ============================================================
// MK Code Runner
// Execute Python, Bash, and C++ code with timeouts.
// Uses fork+exec with "timeout" prefix for safety.
// ============================================================

struct MKRunResult {
    std::string stdoutOutput;
    std::string stderrOutput;
    int exitCode;
    bool timedOut;
    bool success;
};

class MKCodeRunner {
private:
    int timeoutSeconds;
    static const size_t MAX_OUTPUT = 64 * 1024; // 64KB output limit

    // Execute a command with timeout, capture output
    MKRunResult executeWithTimeout(const std::string& cmd) const {
        MKRunResult result;
        result.exitCode = -1;
        result.timedOut = false;
        result.success = false;

        std::string fullCmd = "timeout " + std::to_string(timeoutSeconds) + " " +
                              cmd + " 2>&1";

        std::array<char, 4096> buffer;
        std::string output;

        FILE* pipe = popen(fullCmd.c_str(), "r");
        if (!pipe) {
            result.stderrOutput = "Failed to execute command";
            return result;
        }

        while (fgets(buffer.data(), (int)buffer.size(), pipe) != nullptr) {
            output += buffer.data();
            if (output.size() > MAX_OUTPUT) {
                output += "\n[OUTPUT TRUNCATED - exceeded 64KB limit]";
                break;
            }
        }

        int status = pclose(pipe);
        result.exitCode = WEXITSTATUS(status);

        // timeout command returns 124 on timeout
        if (result.exitCode == 124) {
            result.timedOut = true;
            result.stderrOutput = "Execution timed out after " +
                                  std::to_string(timeoutSeconds) + " seconds";
        }

        result.stdoutOutput = output;
        result.success = (result.exitCode == 0);
        return result;
    }


public:
    MKCodeRunner(int timeout = 10) : timeoutSeconds(timeout) {}

    // Run Python code
    MKRunResult runPython(const std::string& code) const {
        // Write code to temp file
        std::string tmpPath = "/tmp/mk_run.py";
        std::ofstream file(tmpPath);
        if (!file.is_open()) {
            MKRunResult r;
            r.success = false;
            r.stderrOutput = "Cannot write to temp file";
            r.exitCode = -1;
            r.timedOut = false;
            return r;
        }
        file << code;
        file.close();

        return executeWithTimeout("python3 " + tmpPath);
    }

    // Run Bash command
    MKRunResult runBash(const std::string& cmd) const {
        return executeWithTimeout("bash -c '" + cmd + "'");
    }

    // Run C++ code
    MKRunResult runCpp(const std::string& code) const {
        // Write code to temp file
        std::string srcPath = "/tmp/mk_run.cpp";
        std::string binPath = "/tmp/mk_run_bin";

        std::ofstream file(srcPath);
        if (!file.is_open()) {
            MKRunResult r;
            r.success = false;
            r.stderrOutput = "Cannot write to temp file";
            r.exitCode = -1;
            r.timedOut = false;
            return r;
        }
        file << code;
        file.close();

        // Compile
        std::string compileCmd = "g++ -std=c++17 -o " + binPath + " " + srcPath + " 2>&1";
        std::array<char, 4096> buffer;
        std::string compileOutput;

        FILE* pipe = popen(compileCmd.c_str(), "r");
        if (!pipe) {
            MKRunResult r;
            r.success = false;
            r.stderrOutput = "Failed to invoke compiler";
            r.exitCode = -1;
            r.timedOut = false;
            return r;
        }
        while (fgets(buffer.data(), (int)buffer.size(), pipe) != nullptr) {
            compileOutput += buffer.data();
        }
        int compileStatus = pclose(pipe);

        if (WEXITSTATUS(compileStatus) != 0) {
            MKRunResult r;
            r.success = false;
            r.stderrOutput = "Compilation failed:\n" + compileOutput;
            r.exitCode = WEXITSTATUS(compileStatus);
            r.timedOut = false;
            return r;
        }

        // Run the compiled binary
        return executeWithTimeout(binPath);
    }

    // Format run result for display
    std::string formatResult(const MKRunResult& result, const std::string& lang) const {
        std::string output;
        if (result.timedOut) {
            output += "⏱ TIMEOUT: Execution exceeded " + std::to_string(timeoutSeconds) + "s limit\n";
        }
        if (!result.stdoutOutput.empty()) {
            output += result.stdoutOutput;
            if (output.back() != '\n') output += '\n';
        }
        if (!result.stderrOutput.empty() && !result.success) {
            output += "Error: " + result.stderrOutput + "\n";
        }
        if (result.success) {
            output += "[" + lang + " exited with code 0]\n";
        } else if (!result.timedOut) {
            output += "[" + lang + " exited with code " + std::to_string(result.exitCode) + "]\n";
        }
        return output;
    }

    void setTimeout(int seconds) { timeoutSeconds = seconds; }
    int getTimeout() const { return timeoutSeconds; }
};

#endif // MK_CODE_RUNNER_CPP
