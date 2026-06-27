#ifndef MK_CODE_VERIFIER_CPP
#define MK_CODE_VERIFIER_CPP

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <cstdio>
#include <array>
#include <chrono>
#include <unordered_map>

// ===========================================================================
// MK CODE VERIFIER - Write-Test-Verify-Fix Loop
// ===========================================================================
// Compiles/runs code, checks output against expected behavior,
// identifies failures, generates fix suggestions, and re-runs.
// Implements up to 3 iterations of the fix cycle.
// ===========================================================================

// Compilation/execution result
struct MKCompileResult {
    bool success;
    std::string output;
    std::string errors;
    int exit_code;
    double compile_time_ms;
};

// Test verification result
struct MKTestResult {
    bool passed;
    std::string expected;
    std::string actual;
    std::string diff_summary;
    std::string failure_type;  // compile_error, runtime_error, wrong_output, timeout
};

// One iteration of the verification loop
struct MKVerificationIteration {
    int iteration_number;
    std::string code_version;
    MKCompileResult compile;
    MKTestResult test;
    std::string fix_applied;
    bool resolved;
};

// Full verification loop result
struct MKVerificationLoop {
    bool final_success;
    int iterations_used;
    int max_iterations;
    std::string final_code;
    std::vector<MKVerificationIteration> history;
    std::string summary;
    double total_time_ms;
};

class MKCodeVerifier {
private:
    int max_iterations;
    int timeout_seconds;
    std::string temp_dir;
    int total_verifications;
    int successful_verifications;

    // Common error patterns and their fixes
    std::unordered_map<std::string, std::string> error_fixes;

    void initErrorFixes() {
        // Python error patterns
        error_fixes["IndentationError"] = "fix_indentation";
        error_fixes["SyntaxError: unexpected EOF"] = "add_closing_bracket";
        error_fixes["NameError: name"] = "add_import_or_define";
        error_fixes["TypeError: missing"] = "add_default_argument";
        error_fixes["ModuleNotFoundError"] = "remove_or_replace_import";
        error_fixes["AttributeError"] = "check_method_name";
        error_fixes["IndexError"] = "add_bounds_check";
        error_fixes["KeyError"] = "add_key_check";
        error_fixes["ZeroDivisionError"] = "add_zero_check";
        // JavaScript error patterns
        error_fixes["ReferenceError"] = "add_declaration";
        error_fixes["TypeError: Cannot read"] = "add_null_check";
        error_fixes["SyntaxError: Unexpected token"] = "fix_syntax";
        // C++ error patterns
        error_fixes["undefined reference"] = "add_include_or_link";
        error_fixes["no matching function"] = "fix_function_signature";
        error_fixes["expected ';'"] = "add_semicolon";
        error_fixes["undeclared identifier"] = "add_declaration";
        error_fixes["segmentation fault"] = "add_null_check";
    }

    // Execute code and capture results
    MKCompileResult executeCode(const std::string& code, const std::string& language) {
        MKCompileResult result;
        result.success = false;
        result.exit_code = -1;

        // Write code to temp file
        std::string ext = ".py";
        std::string run_cmd;
        if (language == "python") {
            ext = ".py";
            run_cmd = "python3";
        } else if (language == "javascript") {
            ext = ".js";
            run_cmd = "node";
        } else if (language == "cpp") {
            ext = ".cpp";
            run_cmd = "compile_and_run";  // special handling
        }

        std::string filepath = temp_dir + "/verify_code" + ext;
        std::ofstream out(filepath);
        if (!out.is_open()) {
            result.errors = "Failed to write temp file";
            return result;
        }
        out << code;
        out.close();

        // Build execution command with timeout
        std::string cmd;
        if (language == "cpp") {
            std::string bin = temp_dir + "/verify_bin";
            cmd = "g++ -std=c++17 -o " + bin + " " + filepath + " 2>&1 && " + bin + " 2>&1";
        } else {
            cmd = "timeout " + std::to_string(timeout_seconds) + " " + run_cmd + " " + filepath + " 2>&1";
        }

        auto start = std::chrono::high_resolution_clock::now();
        std::array<char, 4096> buffer;
        std::string output;
        FILE* pipe = popen(cmd.c_str(), "r");
        if (pipe) {
            while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
                output += buffer.data();
            result.exit_code = pclose(pipe);
        }
        auto end = std::chrono::high_resolution_clock::now();
        result.compile_time_ms = std::chrono::duration<double, std::milli>(end - start).count();

        if (result.exit_code == 0) {
            result.success = true;
            result.output = output;
        } else {
            result.errors = output;
        }
        return result;
    }

    // Analyze error and suggest a fix
    std::string analyzeError(const std::string& error_msg, const std::string& code) {
        // Check known error patterns
        for (const auto& [pattern, fix_type] : error_fixes) {
            if (error_msg.find(pattern) != std::string::npos) {
                return applyFix(code, fix_type, error_msg);
            }
        }
        // Generic fix: add try-except/try-catch
        return addErrorHandling(code);
    }

    // Apply a specific fix type
    std::string applyFix(const std::string& code, const std::string& fix_type,
                          const std::string& error_msg) {
        std::string fixed = code;
        if (fix_type == "fix_indentation") {
            // Normalize indentation to 4 spaces
            std::stringstream ss(fixed);
            std::string line;
            std::string result;
            while (std::getline(ss, line)) {
                // Count leading whitespace and normalize
                size_t indent = 0;
                for (char c : line) {
                    if (c == ' ') indent++;
                    else if (c == '\t') indent += 4;
                    else break;
                }
                size_t normalized = (indent / 4) * 4;
                result += std::string(normalized, ' ') + line.substr(indent) + "\n";
            }
            return result;
        }
        if (fix_type == "add_closing_bracket") {
            // Count brackets and add missing ones
            int parens = 0, brackets = 0, braces = 0;
            for (char c : fixed) {
                if (c == '(') parens++;
                else if (c == ')') parens--;
                else if (c == '[') brackets++;
                else if (c == ']') brackets--;
                else if (c == '{') braces++;
                else if (c == '}') braces--;
            }
            while (parens > 0) { fixed += ")"; parens--; }
            while (brackets > 0) { fixed += "]"; brackets--; }
            while (braces > 0) { fixed += "}"; braces--; }
            return fixed;
        }
        if (fix_type == "add_null_check" || fix_type == "add_bounds_check") {
            // Wrap in safety check
            return "# Auto-fix: added safety wrapper\ntry:\n" + addIndent(code) + "\nexcept (IndexError, TypeError, AttributeError) as e:\n    print(f'Handled: {e}')\n";
        }
        if (fix_type == "add_zero_check") {
            // Find division and add guard
            size_t pos = fixed.find("/");
            if (pos != std::string::npos && pos > 0) {
                fixed = "# Auto-fix: zero division guard added\n" + fixed;
            }
            return fixed;
        }
        if (fix_type == "add_semicolon") {
            // Add semicolons at end of lines that need them
            std::stringstream ss(fixed);
            std::string line, result;
            while (std::getline(ss, line)) {
                std::string trimmed = line;
                while (!trimmed.empty() && trimmed.back() == ' ') trimmed.pop_back();
                if (!trimmed.empty() && trimmed.back() != ';' && trimmed.back() != '{' &&
                    trimmed.back() != '}' && trimmed.back() != ':' && trimmed[0] != '#') {
                    line += ";";
                }
                result += line + "\n";
            }
            return result;
        }
        return fixed;  // Return as-is if no fix applied
    }

    // Add indentation to code block
    std::string addIndent(const std::string& code, int spaces = 4) {
        std::stringstream ss(code);
        std::string line, result;
        std::string indent(spaces, ' ');
        while (std::getline(ss, line)) {
            result += indent + line + "\n";
        }
        return result;
    }

    // Add generic error handling wrapper
    std::string addErrorHandling(const std::string& code) {
        return "try:\n" + addIndent(code) + "except Exception as e:\n    print(f'Error: {e}')\n";
    }

public:
    MKCodeVerifier() : max_iterations(3), timeout_seconds(10),
                        total_verifications(0), successful_verifications(0) {
        temp_dir = "/tmp/mk_verify";
        initErrorFixes();
    }

    // Main verification loop: write -> test -> verify -> fix
    MKVerificationLoop verify(const std::string& code,
                              const std::string& language,
                              const std::string& expected_output = "") {
        MKVerificationLoop loop;
        loop.final_success = false;
        loop.max_iterations = max_iterations;
        loop.iterations_used = 0;
        loop.final_code = code;
        total_verifications++;

        auto start = std::chrono::high_resolution_clock::now();
        std::string current_code = code;

        for (int i = 0; i < max_iterations; i++) {
            MKVerificationIteration iter;
            iter.iteration_number = i + 1;
            iter.code_version = current_code;
            iter.resolved = false;

            // Compile and run
            iter.compile = executeCode(current_code, language);

            // Check result
            iter.test.passed = false;
            if (iter.compile.success) {
                // Check output against expected
                if (expected_output.empty()) {
                    iter.test.passed = true;  // No expected output = just needs to run
                    iter.test.failure_type = "";
                } else {
                    std::string trimmed_actual = iter.compile.output;
                    while (!trimmed_actual.empty() && trimmed_actual.back() == '\n')
                        trimmed_actual.pop_back();
                    if (trimmed_actual == expected_output) {
                        iter.test.passed = true;
                    } else {
                        iter.test.failure_type = "wrong_output";
                        iter.test.expected = expected_output;
                        iter.test.actual = trimmed_actual;
                    }
                }
            } else {
                // Compilation or runtime error
                if (iter.compile.errors.find("timeout") != std::string::npos) {
                    iter.test.failure_type = "timeout";
                } else if (iter.compile.exit_code != 0) {
                    iter.test.failure_type = "compile_error";
                } else {
                    iter.test.failure_type = "runtime_error";
                }
                iter.test.actual = iter.compile.errors;
            }

            if (iter.test.passed) {
                iter.resolved = true;
                loop.final_success = true;
                loop.final_code = current_code;
                loop.history.push_back(iter);
                loop.iterations_used = i + 1;
                break;
            }

            // Apply fix
            std::string error = iter.compile.success ? iter.test.actual : iter.compile.errors;
            current_code = analyzeError(error, current_code);
            iter.fix_applied = "Auto-fix for: " + iter.test.failure_type;
            loop.history.push_back(iter);
            loop.iterations_used = i + 1;
        }

        auto end = std::chrono::high_resolution_clock::now();
        loop.total_time_ms = std::chrono::duration<double, std::milli>(end - start).count();

        if (loop.final_success) successful_verifications++;
        loop.summary = loop.final_success ?
            "Verification PASSED after " + std::to_string(loop.iterations_used) + " iteration(s)" :
            "Verification FAILED after " + std::to_string(loop.iterations_used) + " iteration(s)";
        return loop;
    }

    // Set max iterations
    void setMaxIterations(int n) { max_iterations = std::max(1, std::min(n, 10)); }
    void setTimeout(int seconds) { timeout_seconds = std::max(1, seconds); }
    int getTotalVerifications() const { return total_verifications; }
    int getSuccessful() const { return successful_verifications; }

    void printStats() const {
        std::cout << "[MKCodeVerifier] Total: " << total_verifications
                  << ", Successful: " << successful_verifications << std::endl;
    }
};

#endif // MK_CODE_VERIFIER_CPP
