#ifndef MK_CODE_EXECUTOR_CPP
#define MK_CODE_EXECUTOR_CPP

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
#include <array>
#include <thread>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

// ============================================================================
// MKCodeExecutor - Runs code in a sandboxed environment
// Supports Python, C++, JavaScript, shell with timeout and output capture
// ============================================================================

enum class ExecLanguage {
    PYTHON,
    CPP,
    JAVASCRIPT,
    SHELL,
    UNKNOWN
};

struct ExecutionResult {
    std::string stdout_output;
    std::string stderr_output;
    int exit_code;
    double execution_time_ms;
    bool timed_out;
    bool compilation_error;
    std::string language;
    std::string source_file;
};

struct SandboxConfig {
    int timeout_seconds;
    size_t max_output_bytes;
    bool allow_network;
    bool allow_file_write;
    std::string temp_directory;
    std::vector<std::string> allowed_commands;
};

class MKCodeExecutor {
private:
    SandboxConfig config_;
    std::vector<ExecutionResult> execution_history_;
    int execution_counter_;
    std::string last_stdout_;
    std::string last_stderr_;

    std::string getTempDir() const {
        return config_.temp_directory.empty() ? "/tmp/mk_executor" : config_.temp_directory;
    }

    std::string generateTempFilename(ExecLanguage lang) const {
        std::string base = getTempDir() + "/mk_exec_" + std::to_string(execution_counter_);
        switch (lang) {
            case ExecLanguage::PYTHON: return base + ".py";
            case ExecLanguage::CPP: return base + ".cpp";
            case ExecLanguage::JAVASCRIPT: return base + ".js";
            case ExecLanguage::SHELL: return base + ".sh";
            default: return base + ".txt";
        }
    }

    bool writeToFile(const std::string& path, const std::string& content) {
        std::ofstream file(path);
        if (!file.is_open()) return false;
        file << content;
        file.close();
        return true;
    }

    std::string readFromFile(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) return "";
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        return content;
    }

    void ensureTempDir() {
        std::string cmd = "mkdir -p " + getTempDir();
        system(cmd.c_str());
    }

    void cleanupFile(const std::string& path) {
        std::remove(path.c_str());
    }

    std::string buildCompileCommand(const std::string& source, const std::string& output) const {
        return "g++ -std=c++17 -o " + output + " " + source + " 2>&1";
    }

    std::string buildRunCommand(const std::string& file, ExecLanguage lang) const {
        switch (lang) {
            case ExecLanguage::PYTHON:
                return "python3 " + file;
            case ExecLanguage::CPP: {
                std::string binary = file.substr(0, file.find_last_of('.'));
                return binary;
            }
            case ExecLanguage::JAVASCRIPT:
                return "node " + file;
            case ExecLanguage::SHELL:
                return "bash " + file;
            default:
                return "";
        }
    }

    ExecutionResult runWithTimeout(const std::string& command, int timeout_sec) {
        ExecutionResult result;
        result.timed_out = false;
        result.compilation_error = false;

        auto start = std::chrono::high_resolution_clock::now();

        // Build command with timeout and output capture
        std::string stdout_file = getTempDir() + "/stdout_" + std::to_string(execution_counter_);
        std::string stderr_file = getTempDir() + "/stderr_" + std::to_string(execution_counter_);

        std::string full_cmd = "timeout " + std::to_string(timeout_sec) + " " 
                             + command + " > " + stdout_file + " 2> " + stderr_file;

        int ret = system(full_cmd.c_str());
        int exit_code = WEXITSTATUS(ret);

        auto end = std::chrono::high_resolution_clock::now();
        result.execution_time_ms = std::chrono::duration<double, std::milli>(end - start).count();

        // Check if timed out (exit code 124 from timeout command)
        if (exit_code == 124) {
            result.timed_out = true;
            result.exit_code = -1;
        } else {
            result.exit_code = exit_code;
        }

        // Read captured output
        result.stdout_output = readFromFile(stdout_file);
        result.stderr_output = readFromFile(stderr_file);

        // Truncate if exceeds max
        if (result.stdout_output.size() > config_.max_output_bytes) {
            result.stdout_output = result.stdout_output.substr(0, config_.max_output_bytes);
            result.stdout_output += "\n[OUTPUT TRUNCATED]";
        }
        if (result.stderr_output.size() > config_.max_output_bytes) {
            result.stderr_output = result.stderr_output.substr(0, config_.max_output_bytes);
            result.stderr_output += "\n[ERROR OUTPUT TRUNCATED]";
        }

        // Cleanup temp files
        cleanupFile(stdout_file);
        cleanupFile(stderr_file);

        return result;
    }

    ExecLanguage detectLanguage(const std::string& code) const {
        if (code.find("def ") != std::string::npos || code.find("import ") != std::string::npos) {
            return ExecLanguage::PYTHON;
        }
        if (code.find("#include") != std::string::npos || code.find("int main") != std::string::npos) {
            return ExecLanguage::CPP;
        }
        if (code.find("console.log") != std::string::npos || code.find("const ") != std::string::npos) {
            return ExecLanguage::JAVASCRIPT;
        }
        if (code.find("#!/bin/bash") != std::string::npos || code.find("echo ") != std::string::npos) {
            return ExecLanguage::SHELL;
        }
        return ExecLanguage::UNKNOWN;
    }

    ExecLanguage parseLanguage(const std::string& lang) const {
        std::string lower = lang;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower == "python" || lower == "py") return ExecLanguage::PYTHON;
        if (lower == "c++" || lower == "cpp") return ExecLanguage::CPP;
        if (lower == "javascript" || lower == "js") return ExecLanguage::JAVASCRIPT;
        if (lower == "shell" || lower == "bash" || lower == "sh") return ExecLanguage::SHELL;
        return ExecLanguage::UNKNOWN;
    }

public:
    MKCodeExecutor() : execution_counter_(0) {
        config_.timeout_seconds = 30;
        config_.max_output_bytes = 65536;
        config_.allow_network = false;
        config_.allow_file_write = true;
        config_.temp_directory = "/tmp/mk_executor";
        ensureTempDir();
    }

    explicit MKCodeExecutor(const SandboxConfig& config) 
        : config_(config), execution_counter_(0) {
        ensureTempDir();
    }

    ExecutionResult execute(const std::string& code, const std::string& language) {
        execution_counter_++;
        ExecLanguage lang = parseLanguage(language);
        if (lang == ExecLanguage::UNKNOWN) {
            lang = detectLanguage(code);
        }

        std::string source_file = generateTempFilename(lang);
        writeToFile(source_file, code);

        ExecutionResult result;
        result.source_file = source_file;
        result.language = language;

        // C++ needs compilation first
        if (lang == ExecLanguage::CPP) {
            std::string binary = source_file.substr(0, source_file.find_last_of('.'));
            std::string compile_cmd = buildCompileCommand(source_file, binary);
            
            std::string compile_output_file = getTempDir() + "/compile_" + std::to_string(execution_counter_);
            std::string full_compile = compile_cmd + " > " + compile_output_file + " 2>&1";
            int compile_ret = system(full_compile.c_str());

            if (WEXITSTATUS(compile_ret) != 0) {
                result.compilation_error = true;
                result.stderr_output = readFromFile(compile_output_file);
                result.exit_code = WEXITSTATUS(compile_ret);
                result.execution_time_ms = 0;
                result.timed_out = false;
                cleanupFile(compile_output_file);
                execution_history_.push_back(result);
                return result;
            }
            cleanupFile(compile_output_file);
        }

        std::string run_cmd = buildRunCommand(source_file, lang);
        result = runWithTimeout(run_cmd, config_.timeout_seconds);
        result.source_file = source_file;
        result.language = language;

        last_stdout_ = result.stdout_output;
        last_stderr_ = result.stderr_output;
        execution_history_.push_back(result);

        return result;
    }

    ExecutionResult executeFile(const std::string& path) {
        std::string code = readFromFile(path);
        if (code.empty()) {
            ExecutionResult err;
            err.exit_code = -1;
            err.stderr_output = "Could not read file: " + path;
            return err;
        }

        // Detect language from extension
        std::string ext = path.substr(path.find_last_of('.'));
        std::string lang = "unknown";
        if (ext == ".py") lang = "python";
        else if (ext == ".cpp" || ext == ".cc") lang = "cpp";
        else if (ext == ".js") lang = "javascript";
        else if (ext == ".sh") lang = "shell";

        return execute(code, lang);
    }

    std::string getOutput() const { return last_stdout_; }
    std::string getErrors() const { return last_stderr_; }

    void setTimeout(int seconds) {
        config_.timeout_seconds = std::max(1, std::min(300, seconds));
    }

    void setMaxOutputSize(size_t bytes) {
        config_.max_output_bytes = bytes;
    }

    SandboxConfig getConfig() const { return config_; }
    int getExecutionCount() const { return execution_counter_; }

    std::vector<ExecutionResult> getHistory() const { return execution_history_; }

    void clearHistory() {
        execution_history_.clear();
    }

    std::string getLanguageName(ExecLanguage lang) const {
        switch (lang) {
            case ExecLanguage::PYTHON: return "Python";
            case ExecLanguage::CPP: return "C++";
            case ExecLanguage::JAVASCRIPT: return "JavaScript";
            case ExecLanguage::SHELL: return "Shell";
            default: return "Unknown";
        }
    }
};

#endif // MK_CODE_EXECUTOR_CPP
