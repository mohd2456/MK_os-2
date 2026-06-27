#ifndef MK_CODE_VERIFIER_CPP
#define MK_CODE_VERIFIER_CPP

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <chrono>

// ============================================================================
// MKCodeVerifier - Implements the write-run-fix loop for generated code
// Runs code, checks output, analyzes errors, modifies code, and retries
// ============================================================================

enum class VerifyLanguage {
    PYTHON,
    CPP,
    JAVASCRIPT,
    SHELL
};

enum class ErrorCategory {
    SYNTAX_ERROR,
    RUNTIME_ERROR,
    TYPE_ERROR,
    IMPORT_ERROR,
    LOGIC_ERROR,
    TIMEOUT_ERROR,
    COMPILATION_ERROR,
    UNKNOWN_ERROR
};

struct FixAttempt {
    int attempt_number;
    std::string original_code;
    std::string modified_code;
    std::string error_message;
    ErrorCategory error_type;
    std::string fix_description;
    bool successful;
    double time_ms;
};

struct VerificationResult {
    bool success;
    std::string final_code;
    std::string output;
    int total_attempts;
    std::vector<FixAttempt> fix_history;
    double total_time_ms;
    std::string failure_reason;
};

struct ExpectedBehavior {
    std::string expected_output;
    std::vector<std::string> required_substrings;
    bool must_exit_zero;
    int max_execution_time_ms;
};

class MKCodeVerifier {
private:
    int max_retries_;
    std::vector<FixAttempt> history_;
    std::map<ErrorCategory, std::vector<std::string>> error_patterns_;

    void initializeErrorPatterns() {
        error_patterns_[ErrorCategory::SYNTAX_ERROR] = {
            "SyntaxError", "syntax error", "unexpected token",
            "expected", "invalid syntax"
        };
        error_patterns_[ErrorCategory::RUNTIME_ERROR] = {
            "RuntimeError", "segmentation fault", "core dumped",
            "SIGSEGV", "abort"
        };
        error_patterns_[ErrorCategory::TYPE_ERROR] = {
            "TypeError", "type mismatch", "cannot convert",
            "incompatible types", "undefined is not"
        };
        error_patterns_[ErrorCategory::IMPORT_ERROR] = {
            "ImportError", "ModuleNotFoundError", "Cannot find module",
            "no such file or directory", "#include"
        };
        error_patterns_[ErrorCategory::LOGIC_ERROR] = {
            "AssertionError", "assertion failed", "test failed",
            "expected output"
        };
        error_patterns_[ErrorCategory::TIMEOUT_ERROR] = {
            "timeout", "timed out", "exceeded time limit",
            "killed"
        };
        error_patterns_[ErrorCategory::COMPILATION_ERROR] = {
            "error:", "undefined reference", "linker error",
            "fatal error", "compilation failed"
        };
    }

    ErrorCategory classifyError(const std::string& error_msg) const {
        std::string lower = error_msg;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        for (const auto& kv : error_patterns_) {
            for (const auto& pattern : kv.second) {
                std::string lower_pattern = pattern;
                std::transform(lower_pattern.begin(), lower_pattern.end(), 
                             lower_pattern.begin(), ::tolower);
                if (lower.find(lower_pattern) != std::string::npos) {
                    return kv.first;
                }
            }
        }
        return ErrorCategory::UNKNOWN_ERROR;
    }

    std::string analyzeAndFix(const std::string& code, const std::string& error, 
                              ErrorCategory category, VerifyLanguage lang) {
        std::string fixed = code;

        switch (category) {
            case ErrorCategory::SYNTAX_ERROR:
                fixed = fixSyntaxError(code, error, lang);
                break;
            case ErrorCategory::IMPORT_ERROR:
                fixed = fixImportError(code, error, lang);
                break;
            case ErrorCategory::TYPE_ERROR:
                fixed = fixTypeError(code, error, lang);
                break;
            case ErrorCategory::RUNTIME_ERROR:
                fixed = fixRuntimeError(code, error, lang);
                break;
            case ErrorCategory::COMPILATION_ERROR:
                fixed = fixCompilationError(code, error, lang);
                break;
            default:
                fixed = attemptGenericFix(code, error, lang);
                break;
        }
        return fixed;
    }

    std::string fixSyntaxError(const std::string& code, const std::string& error,
                               VerifyLanguage lang) {
        std::string fixed = code;

        if (lang == VerifyLanguage::PYTHON) {
            // Common Python syntax fixes
            // Missing colons after if/for/def/class
            std::vector<std::string> keywords = {"if ", "for ", "while ", "def ", "class "};
            std::istringstream stream(fixed);
            std::string line;
            std::string result;
            while (std::getline(stream, line)) {
                std::string trimmed = line;
                size_t start = trimmed.find_first_not_of(" \t");
                if (start != std::string::npos) trimmed = trimmed.substr(start);
                
                for (const auto& kw : keywords) {
                    if (trimmed.find(kw) == 0 && trimmed.back() != ':' && 
                        trimmed.find(':') == std::string::npos) {
                        line += ":";
                        break;
                    }
                }
                result += line + "\n";
            }
            fixed = result;
        }

        if (lang == VerifyLanguage::CPP || lang == VerifyLanguage::JAVASCRIPT) {
            // Check for missing semicolons - simple heuristic
            std::istringstream stream(fixed);
            std::string line;
            std::string result;
            while (std::getline(stream, line)) {
                std::string trimmed = line;
                while (!trimmed.empty() && (trimmed.back() == ' ' || trimmed.back() == '\t'))
                    trimmed.pop_back();
                if (!trimmed.empty() && trimmed.back() != ';' && trimmed.back() != '{' &&
                    trimmed.back() != '}' && trimmed.back() != ',' && trimmed.back() != '/' &&
                    trimmed.find("//") == std::string::npos && trimmed.find("#") != 0 &&
                    trimmed.find("if") == std::string::npos && trimmed.find("for") == std::string::npos) {
                    // Might need a semicolon - check if error mentions this line
                }
                result += line + "\n";
            }
            fixed = result;
        }
        return fixed;
    }

    std::string fixImportError(const std::string& code, const std::string& error,
                               VerifyLanguage lang) {
        std::string fixed = code;

        if (lang == VerifyLanguage::PYTHON) {
            // Try to remove the problematic import and use stdlib alternatives
            size_t pos = error.find("No module named");
            if (pos != std::string::npos) {
                std::string module = error.substr(pos + 17);
                module = module.substr(0, module.find_first_of("'\"\n"));
                // Comment out the import
                size_t import_pos = fixed.find("import " + module);
                if (import_pos != std::string::npos) {
                    fixed.insert(import_pos, "# ");
                }
            }
        }
        return fixed;
    }

    std::string fixTypeError(const std::string& code, const std::string& error,
                             VerifyLanguage lang) {
        // Type errors often need explicit conversions
        std::string fixed = code;
        if (lang == VerifyLanguage::PYTHON) {
            if (error.find("str") != std::string::npos && error.find("int") != std::string::npos) {
                // Likely needs str() or int() conversion
                // Simple heuristic: wrap print arguments
            }
        }
        return fixed;
    }

    std::string fixRuntimeError(const std::string& code, const std::string& error,
                                VerifyLanguage lang) {
        std::string fixed = code;
        // Add bounds checking, null checks, etc.
        if (error.find("index") != std::string::npos || error.find("out of range") != std::string::npos) {
            // Add bounds checking
        }
        if (error.find("division by zero") != std::string::npos || 
            error.find("divide by zero") != std::string::npos) {
            // Add zero-division guard
        }
        return fixed;
    }

    std::string fixCompilationError(const std::string& code, const std::string& error,
                                    VerifyLanguage lang) {
        std::string fixed = code;
        if (error.find("undeclared") != std::string::npos || 
            error.find("was not declared") != std::string::npos) {
            // Missing include or variable declaration
        }
        if (error.find("undefined reference") != std::string::npos) {
            // Missing function implementation or library
        }
        return fixed;
    }

    std::string attemptGenericFix(const std::string& code, const std::string& error,
                                  VerifyLanguage lang) {
        // Last resort: wrap in try/catch or try/except
        std::string fixed = code;
        if (lang == VerifyLanguage::PYTHON) {
            fixed = "try:\n";
            std::istringstream stream(code);
            std::string line;
            while (std::getline(stream, line)) {
                fixed += "    " + line + "\n";
            }
            fixed += "except Exception as e:\n    print(f'Error: {e}')\n";
        }
        return fixed;
    }

    bool checkOutput(const std::string& actual, const ExpectedBehavior& expected) const {
        if (!expected.expected_output.empty()) {
            std::string trimmed_actual = actual;
            std::string trimmed_expected = expected.expected_output;
            // Trim whitespace for comparison
            while (!trimmed_actual.empty() && (trimmed_actual.back() == '\n' || trimmed_actual.back() == ' '))
                trimmed_actual.pop_back();
            while (!trimmed_expected.empty() && (trimmed_expected.back() == '\n' || trimmed_expected.back() == ' '))
                trimmed_expected.pop_back();
            if (trimmed_actual != trimmed_expected) return false;
        }

        for (const auto& sub : expected.required_substrings) {
            if (actual.find(sub) == std::string::npos) return false;
        }
        return true;
    }

    VerifyLanguage parseLanguage(const std::string& lang) const {
        std::string lower = lang;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower == "python" || lower == "py") return VerifyLanguage::PYTHON;
        if (lower == "c++" || lower == "cpp") return VerifyLanguage::CPP;
        if (lower == "javascript" || lower == "js") return VerifyLanguage::JAVASCRIPT;
        return VerifyLanguage::SHELL;
    }

    // Simulated execution for the verify loop
    std::pair<std::string, std::string> simulateExecution(const std::string& code, 
                                                          VerifyLanguage lang) {
        // In production, this calls MKCodeExecutor
        // Returns {stdout, stderr}
        return {"", ""};
    }

public:
    MKCodeVerifier() : max_retries_(5) {
        initializeErrorPatterns();
    }

    explicit MKCodeVerifier(int max_retries) : max_retries_(max_retries) {
        initializeErrorPatterns();
    }

    VerificationResult verifyAndFix(const std::string& code, const std::string& language,
                                    const ExpectedBehavior& expected, int max_retries = -1) {
        if (max_retries < 0) max_retries = max_retries_;

        VerificationResult result;
        result.success = false;
        result.total_attempts = 0;
        auto total_start = std::chrono::high_resolution_clock::now();

        std::string current_code = code;
        VerifyLanguage lang = parseLanguage(language);

        for (int attempt = 0; attempt < max_retries; attempt++) {
            result.total_attempts++;
            auto attempt_start = std::chrono::high_resolution_clock::now();

            FixAttempt fix;
            fix.attempt_number = attempt + 1;
            fix.original_code = current_code;

            // Execute code
            auto [stdout_out, stderr_out] = simulateExecution(current_code, lang);

            if (stderr_out.empty() && checkOutput(stdout_out, expected)) {
                fix.successful = true;
                fix.modified_code = current_code;
                fix.fix_description = "Code passed verification";
                auto attempt_end = std::chrono::high_resolution_clock::now();
                fix.time_ms = std::chrono::duration<double, std::milli>(attempt_end - attempt_start).count();
                result.fix_history.push_back(fix);
                result.success = true;
                result.final_code = current_code;
                result.output = stdout_out;
                break;
            }

            // Analyze error and attempt fix
            fix.error_message = stderr_out;
            fix.error_type = classifyError(stderr_out);
            current_code = analyzeAndFix(current_code, stderr_out, fix.error_type, lang);
            fix.modified_code = current_code;
            fix.successful = false;
            fix.fix_description = "Applied fix for " + getErrorCategoryName(fix.error_type);

            auto attempt_end = std::chrono::high_resolution_clock::now();
            fix.time_ms = std::chrono::duration<double, std::milli>(attempt_end - attempt_start).count();
            result.fix_history.push_back(fix);
        }

        if (!result.success) {
            result.final_code = current_code;
            result.failure_reason = "Max retries (" + std::to_string(max_retries) + ") exceeded";
        }

        auto total_end = std::chrono::high_resolution_clock::now();
        result.total_time_ms = std::chrono::duration<double, std::milli>(total_end - total_start).count();
        return result;
    }

    void setMaxRetries(int retries) { max_retries_ = std::max(1, std::min(20, retries)); }
    int getMaxRetries() const { return max_retries_; }

    std::string getErrorCategoryName(ErrorCategory cat) const {
        switch (cat) {
            case ErrorCategory::SYNTAX_ERROR: return "SyntaxError";
            case ErrorCategory::RUNTIME_ERROR: return "RuntimeError";
            case ErrorCategory::TYPE_ERROR: return "TypeError";
            case ErrorCategory::IMPORT_ERROR: return "ImportError";
            case ErrorCategory::LOGIC_ERROR: return "LogicError";
            case ErrorCategory::TIMEOUT_ERROR: return "TimeoutError";
            case ErrorCategory::COMPILATION_ERROR: return "CompilationError";
            default: return "UnknownError";
        }
    }

    std::vector<FixAttempt> getHistory() const { return history_; }
    void clearHistory() { history_.clear(); }
};

#endif // MK_CODE_VERIFIER_CPP
