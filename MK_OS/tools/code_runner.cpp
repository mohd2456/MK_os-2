#ifndef MK_CODE_RUNNER_CPP
#define MK_CODE_RUNNER_CPP

#include <string>
#include <fstream>
#include <cstdio>
#include <array>
#include <chrono>
#include <iostream>
#include <vector>
#include <algorithm>

// ============================================================
// MK Code Runner
// Execute Python, Bash, C++, JavaScript/Node.js, and Rust code
// with timeouts, input sanitization, and runtime measurement.
// ============================================================

struct MKRunResult {
    std::string stdoutOutput;
    std::string stderrOutput;
    int exitCode;
    bool timedOut;
    bool success;
    double runtimeMs;    // Execution time in milliseconds
    std::string language;  // Which language was used
};

class MKCodeRunner {
private:
    int timeoutSeconds;
    static const size_t MAX_OUTPUT = 64 * 1024; // 64KB output limit

    // Dangerous command patterns to block
    bool isDangerous(const std::string& code) const {
        std::string lower = code;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        // Block dangerous shell patterns
        static const std::vector<std::string> patterns = {
            "rm -rf /", "rm -rf /*", ":(){ :|:& };:",  // fork bomb
            "dd if=/dev/", "mkfs.", "chmod -r 777 /",
            "wget http", "curl http",  // no arbitrary downloads during code execution
            "> /dev/sda", "shutdown", "reboot", "halt",
            "mv /* ", "rm -rf ~", "rm -rf $HOME",
            "/dev/null >", ">/dev/sda"
        };

        for (const auto& p : patterns) {
            if (lower.find(p) != std::string::npos) return true;
        }
        return false;
    }

    // Execute a command with timeout, capture output, measure time
    MKRunResult executeWithTimeout(const std::string& cmd) const {
        MKRunResult result;
        result.exitCode = -1;
        result.timedOut = false;
        result.success = false;
        result.runtimeMs = 0.0;

        std::string fullCmd = "timeout " + std::to_string(timeoutSeconds) + " " +
                              cmd + " 2>&1";

        std::array<char, 4096> buffer;
        std::string output;

        auto startTime = std::chrono::steady_clock::now();

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

        auto endTime = std::chrono::steady_clock::now();
        result.runtimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();

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

    // Detect language from code content
    std::string detectLanguage(const std::string& code) const {
        std::string lower = code;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        if (lower.find("#include") != std::string::npos || lower.find("int main") != std::string::npos ||
            lower.find("std::") != std::string::npos || lower.find("iostream") != std::string::npos) {
            return "cpp";
        }
        if (lower.find("fn main") != std::string::npos || lower.find("let mut") != std::string::npos ||
            lower.find("println!") != std::string::npos || lower.find("use std") != std::string::npos) {
            return "rust";
        }
        if (lower.find("console.log") != std::string::npos || lower.find("const ") != std::string::npos ||
            lower.find("require(") != std::string::npos || lower.find("=> {") != std::string::npos ||
            lower.find("function ") != std::string::npos || lower.find("var ") != std::string::npos) {
            return "javascript";
        }
        if (lower.find("#!/bin/bash") != std::string::npos || lower.find("#!/bin/sh") != std::string::npos) {
            return "bash";
        }
        if (lower.find("def ") != std::string::npos || lower.find("import ") != std::string::npos ||
            lower.find("print(") != std::string::npos || lower.find("class ") != std::string::npos) {
            return "python";
        }
        // Default to python
        return "python";
    }

public:
    MKCodeRunner(int timeout = 10) : timeoutSeconds(timeout) {}

    // Auto-detect language and run
    MKRunResult run(const std::string& code) const {
        std::string lang = detectLanguage(code);
        return run(code, lang);
    }

    // Run with explicit language specification
    MKRunResult run(const std::string& code, const std::string& lang) const {
        MKRunResult result;
        result.language = lang;

        // Safety check
        if (isDangerous(code)) {
            result.success = false;
            result.exitCode = -1;
            result.timedOut = false;
            result.runtimeMs = 0;
            result.stderrOutput = "BLOCKED: Code contains potentially dangerous commands. "
                                  "Operations like rm -rf, fork bombs, and disk writes are not allowed.";
            return result;
        }

        if (lang == "python") return runPython(code);
        if (lang == "bash" || lang == "sh") return runBash(code);
        if (lang == "cpp" || lang == "c++") return runCpp(code);
        if (lang == "javascript" || lang == "js" || lang == "node") return runJavaScript(code);
        if (lang == "rust" || lang == "rs") return runRust(code);

        result.success = false;
        result.exitCode = -1;
        result.timedOut = false;
        result.runtimeMs = 0;
        result.stderrOutput = "Unsupported language: " + lang +
                              ". Supported: python, bash, c++, javascript, rust";
        return result;
    }

    // Run Python code
    MKRunResult runPython(const std::string& code) const {
        if (isDangerous(code)) {
            MKRunResult r; r.success = false; r.exitCode = -1; r.timedOut = false; r.runtimeMs = 0;
            r.stderrOutput = "BLOCKED: Dangerous command detected"; r.language = "python"; return r;
        }

        std::string tmpPath = "/tmp/mk_run.py";
        std::ofstream file(tmpPath);
        if (!file.is_open()) {
            MKRunResult r; r.success = false; r.stderrOutput = "Cannot write to temp file";
            r.exitCode = -1; r.timedOut = false; r.runtimeMs = 0; r.language = "python"; return r;
        }
        file << code;
        file.close();

        MKRunResult r = executeWithTimeout("python3 " + tmpPath);
        r.language = "python";
        return r;
    }

    // Run Bash command
    MKRunResult runBash(const std::string& cmd) const {
        if (isDangerous(cmd)) {
            MKRunResult r; r.success = false; r.exitCode = -1; r.timedOut = false; r.runtimeMs = 0;
            r.stderrOutput = "BLOCKED: Dangerous command detected"; r.language = "bash"; return r;
        }

        std::string tmpPath = "/tmp/mk_run.sh";
        std::ofstream file(tmpPath);
        if (!file.is_open()) {
            MKRunResult r; r.success = false; r.stderrOutput = "Cannot write to temp file";
            r.exitCode = -1; r.timedOut = false; r.runtimeMs = 0; r.language = "bash"; return r;
        }
        file << "#!/bin/bash\n" << cmd;
        file.close();

        MKRunResult r = executeWithTimeout("bash " + tmpPath);
        r.language = "bash";
        return r;
    }

    // Run C++ code
    MKRunResult runCpp(const std::string& code) const {
        if (isDangerous(code)) {
            MKRunResult r; r.success = false; r.exitCode = -1; r.timedOut = false; r.runtimeMs = 0;
            r.stderrOutput = "BLOCKED: Dangerous command detected"; r.language = "c++"; return r;
        }

        std::string srcPath = "/tmp/mk_run.cpp";
        std::string binPath = "/tmp/mk_run_bin";

        std::ofstream file(srcPath);
        if (!file.is_open()) {
            MKRunResult r; r.success = false; r.stderrOutput = "Cannot write to temp file";
            r.exitCode = -1; r.timedOut = false; r.runtimeMs = 0; r.language = "c++"; return r;
        }
        file << code;
        file.close();

        // Compile
        std::string compileCmd = "g++ -std=c++17 -o " + binPath + " " + srcPath + " 2>&1";
        std::array<char, 4096> buffer;
        std::string compileOutput;

        FILE* pipe = popen(compileCmd.c_str(), "r");
        if (!pipe) {
            MKRunResult r; r.success = false; r.stderrOutput = "Failed to invoke compiler";
            r.exitCode = -1; r.timedOut = false; r.runtimeMs = 0; r.language = "c++"; return r;
        }
        while (fgets(buffer.data(), (int)buffer.size(), pipe) != nullptr) {
            compileOutput += buffer.data();
        }
        int compileStatus = pclose(pipe);

        if (WEXITSTATUS(compileStatus) != 0) {
            MKRunResult r; r.success = false;
            r.stderrOutput = "Compilation failed:\n" + compileOutput;
            r.exitCode = WEXITSTATUS(compileStatus); r.timedOut = false; r.runtimeMs = 0;
            r.language = "c++"; return r;
        }

        // Run the compiled binary
        MKRunResult r = executeWithTimeout(binPath);
        r.language = "c++";
        return r;
    }

    // Run JavaScript/Node.js code
    MKRunResult runJavaScript(const std::string& code) const {
        if (isDangerous(code)) {
            MKRunResult r; r.success = false; r.exitCode = -1; r.timedOut = false; r.runtimeMs = 0;
            r.stderrOutput = "BLOCKED: Dangerous command detected"; r.language = "javascript"; return r;
        }

        std::string tmpPath = "/tmp/mk_run.js";
        std::ofstream file(tmpPath);
        if (!file.is_open()) {
            MKRunResult r; r.success = false; r.stderrOutput = "Cannot write to temp file";
            r.exitCode = -1; r.timedOut = false; r.runtimeMs = 0; r.language = "javascript"; return r;
        }
        file << code;
        file.close();

        MKRunResult r = executeWithTimeout("node " + tmpPath);
        r.language = "javascript";
        return r;
    }

    // Run Rust code
    MKRunResult runRust(const std::string& code) const {
        if (isDangerous(code)) {
            MKRunResult r; r.success = false; r.exitCode = -1; r.timedOut = false; r.runtimeMs = 0;
            r.stderrOutput = "BLOCKED: Dangerous command detected"; r.language = "rust"; return r;
        }

        std::string srcPath = "/tmp/mk_run.rs";
        std::string binPath = "/tmp/mk_run_rs";

        std::ofstream file(srcPath);
        if (!file.is_open()) {
            MKRunResult r; r.success = false; r.stderrOutput = "Cannot write to temp file";
            r.exitCode = -1; r.timedOut = false; r.runtimeMs = 0; r.language = "rust"; return r;
        }
        file << code;
        file.close();

        // Compile with rustc
        std::string compileCmd = "rustc -o " + binPath + " " + srcPath + " 2>&1";
        std::array<char, 4096> buffer;
        std::string compileOutput;

        FILE* pipe = popen(compileCmd.c_str(), "r");
        if (!pipe) {
            MKRunResult r; r.success = false; r.stderrOutput = "Failed to invoke rustc compiler";
            r.exitCode = -1; r.timedOut = false; r.runtimeMs = 0; r.language = "rust"; return r;
        }
        while (fgets(buffer.data(), (int)buffer.size(), pipe) != nullptr) {
            compileOutput += buffer.data();
        }
        int compileStatus = pclose(pipe);

        if (WEXITSTATUS(compileStatus) != 0) {
            MKRunResult r; r.success = false;
            r.stderrOutput = "Rust compilation failed:\n" + compileOutput;
            r.exitCode = WEXITSTATUS(compileStatus); r.timedOut = false; r.runtimeMs = 0;
            r.language = "rust"; return r;
        }

        // Run the compiled binary
        MKRunResult r = executeWithTimeout(binPath);
        r.language = "rust";
        return r;
    }

    // Format run result for display
    std::string formatResult(const MKRunResult& result, const std::string& lang) const {
        std::string output;
        if (result.timedOut) {
            output += "TIMEOUT: Execution exceeded " + std::to_string(timeoutSeconds) + "s limit\n";
        }
        if (!result.stdoutOutput.empty()) {
            output += result.stdoutOutput;
            if (output.back() != '\n') output += '\n';
        }
        if (!result.stderrOutput.empty() && !result.success) {
            output += "Error: " + result.stderrOutput + "\n";
        }
        if (result.success) {
            output += "[" + lang + " exited with code 0";
            if (result.runtimeMs > 0) {
                output += " | runtime: ";
                if (result.runtimeMs < 1000) {
                    output += std::to_string((int)result.runtimeMs) + "ms";
                } else {
                    char buf[32];
                    std::snprintf(buf, sizeof(buf), "%.2f", result.runtimeMs / 1000.0);
                    output += std::string(buf) + "s";
                }
            }
            output += "]\n";
        } else if (!result.timedOut) {
            output += "[" + lang + " exited with code " + std::to_string(result.exitCode) + "]\n";
        }
        return output;
    }

    // Get list of supported languages
    static std::vector<std::string> supportedLanguages() {
        return {"python", "bash", "c++", "javascript", "rust"};
    }

    void setTimeout(int seconds) { timeoutSeconds = seconds; }
    int getTimeout() const { return timeoutSeconds; }
};

#endif // MK_CODE_RUNNER_CPP
