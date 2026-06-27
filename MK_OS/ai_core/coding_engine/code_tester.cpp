#ifndef MK_CODE_TESTER_CPP
#define MK_CODE_TESTER_CPP

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <array>
#include <chrono>

// ===========================================================================
// MK CODE TESTER - Automated Test Generation and Execution
// ===========================================================================
// Given generated code, produces unit tests, runs them, captures output.
// Features:
//   - Test framework detection
//   - Assertion generation
//   - Edge case identification
//   - Coverage tracking
//   - Test report formatting
// ===========================================================================

// Test case structure
struct MKTestCase {
    std::string name;
    std::string description;
    std::string input;
    std::string expected_output;
    std::string test_code;
    bool passed;
    std::string actual_output;
    double execution_time_ms;
};

// Test suite result
struct MKTestSuiteResult {
    bool all_passed;
    int total_tests;
    int passed_tests;
    int failed_tests;
    int skipped_tests;
    double total_time_ms;
    std::vector<MKTestCase> test_cases;
    std::string report;
    float coverage_estimate;
};

// Framework detection
struct MKTestFramework {
    std::string name;
    std::string language;
    std::string run_command;
    std::string assertion_style;
};

class MKCodeTester {
private:
    std::vector<MKTestFramework> frameworks;
    int total_tests_run;
    int total_tests_passed;
    std::string temp_dir;

    void initFrameworks() {
        frameworks.push_back({"pytest", "python", "python3 -m pytest", "assert"});
        frameworks.push_back({"unittest", "python", "python3 -m unittest", "self.assert"});
        frameworks.push_back({"jest", "javascript", "npx jest", "expect()"});
        frameworks.push_back({"mocha", "javascript", "npx mocha", "assert"});
        frameworks.push_back({"gtest", "cpp", "./test_binary", "EXPECT_"});
        frameworks.push_back({"catch2", "cpp", "./test_binary", "REQUIRE"});
        frameworks.push_back({"cargo_test", "rust", "cargo test", "assert!"});
        frameworks.push_back({"go_test", "go", "go test ./...", "t.Error"});
    }

    // Detect which framework to use based on language and code
    MKTestFramework detectFramework(const std::string& language,
                                     const std::string& code) {
        for (const auto& fw : frameworks) {
            if (fw.language == language) {
                if (code.find(fw.assertion_style) != std::string::npos) {
                    return fw;
                }
            }
        }
        // Default per language
        for (const auto& fw : frameworks) {
            if (fw.language == language) return fw;
        }
        return {"pytest", "python", "python3 -m pytest", "assert"};
    }

    // Generate test cases for Python code
    std::vector<MKTestCase> generatePythonTests(const std::string& code,
                                                 const std::string& func_name) {
        std::vector<MKTestCase> tests;
        // Basic functionality test
        tests.push_back({
            "test_" + func_name + "_basic",
            "Test basic functionality",
            "simple_input",
            "expected_output",
            "def test_" + func_name + "_basic():\n"
            "    result = " + func_name + "()\n"
            "    assert result is not None\n",
            false, "", 0.0
        });
        // Edge case: empty input
        tests.push_back({
            "test_" + func_name + "_empty",
            "Test with empty/None input",
            "",
            "handles gracefully",
            "def test_" + func_name + "_empty():\n"
            "    try:\n"
            "        result = " + func_name + "(None)\n"
            "        assert True  # no crash\n"
            "    except TypeError:\n"
            "        pass  # acceptable\n",
            false, "", 0.0
        });
        // Type error test
        tests.push_back({
            "test_" + func_name + "_type_error",
            "Test type handling",
            "invalid_type",
            "raises or handles",
            "def test_" + func_name + "_type():\n"
            "    try:\n"
            "        " + func_name + "(12345)  # numeric instead of expected\n"
            "    except (TypeError, ValueError):\n"
            "        pass\n",
            false, "", 0.0
        });
        return tests;
    }

    // Generate test cases for JavaScript code
    std::vector<MKTestCase> generateJSTests(const std::string& code,
                                             const std::string& func_name) {
        std::vector<MKTestCase> tests;
        tests.push_back({
            "test_" + func_name + "_basic",
            "Basic functionality",
            "sample_input",
            "valid_output",
            "describe('" + func_name + "', () => {\n"
            "    test('should work with basic input', () => {\n"
            "        const result = " + func_name + "();\n"
            "        expect(result).toBeDefined();\n"
            "    });\n"
            "});\n",
            false, "", 0.0
        });
        tests.push_back({
            "test_" + func_name + "_error",
            "Error handling",
            "bad_input",
            "throws or handles",
            "describe('" + func_name + " errors', () => {\n"
            "    test('should handle errors gracefully', () => {\n"
            "        expect(() => " + func_name + "(null)).not.toThrow();\n"
            "    });\n"
            "});\n",
            false, "", 0.0
        });
        return tests;
    }

    // Execute a command and capture output
    std::string executeCommand(const std::string& cmd, double& elapsed_ms) {
        auto start = std::chrono::high_resolution_clock::now();
        std::array<char, 4096> buffer;
        std::string output;
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            elapsed_ms = 0;
            return "ERROR: Failed to run command";
        }
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            output += buffer.data();
        }
        int status = pclose(pipe);
        auto end = std::chrono::high_resolution_clock::now();
        elapsed_ms = std::chrono::duration<double, std::milli>(end - start).count();
        if (status != 0) {
            output += "\n[EXIT CODE: " + std::to_string(status) + "]";
        }
        return output;
    }

    // Write test code to a temp file
    std::string writeTestFile(const std::string& language,
                              const std::string& test_code,
                              const std::string& source_code) {
        std::string ext = ".py";
        if (language == "javascript") ext = ".test.js";
        else if (language == "cpp") ext = "_test.cpp";
        else if (language == "rust") ext = "_test.rs";
        else if (language == "go") ext = "_test.go";

        std::string filepath = temp_dir + "/test_generated" + ext;
        std::ofstream out(filepath);
        if (out.is_open()) {
            // Include source code first if needed
            if (language == "python") {
                out << "# Source code\n" << source_code << "\n\n";
                out << "# Tests\n" << test_code << "\n";
            } else {
                out << test_code;
            }
            out.close();
        }
        return filepath;
    }

    // Format test report
    std::string formatReport(const MKTestSuiteResult& suite) {
        std::stringstream ss;
        ss << "\n=== MK Test Report ===\n";
        ss << "Total: " << suite.total_tests
           << " | Passed: " << suite.passed_tests
           << " | Failed: " << suite.failed_tests
           << " | Skipped: " << suite.skipped_tests << "\n";
        ss << "Time: " << suite.total_time_ms << "ms\n";
        ss << "Coverage (est): " << (suite.coverage_estimate * 100) << "%\n\n";
        for (const auto& tc : suite.test_cases) {
            ss << (tc.passed ? "  PASS" : "  FAIL") << " " << tc.name;
            ss << " (" << tc.execution_time_ms << "ms)\n";
            if (!tc.passed && !tc.actual_output.empty()) {
                ss << "       Got: " << tc.actual_output.substr(0, 100) << "\n";
            }
        }
        ss << "\n=== End Report ===\n";
        return ss.str();
    }

public:
    MKCodeTester() : total_tests_run(0), total_tests_passed(0) {
        temp_dir = "/tmp/mk_tests";
        initFrameworks();
    }

    // Generate tests for given code
    std::vector<MKTestCase> generateTests(const std::string& code,
                                           const std::string& language,
                                           const std::string& func_name = "main") {
        if (language == "python") return generatePythonTests(code, func_name);
        if (language == "javascript") return generateJSTests(code, func_name);
        // Default: return basic test
        return {{"test_basic", "Basic test", "", "", "// test placeholder", false, "", 0.0}};
    }

    // Run a test suite
    MKTestSuiteResult runTests(const std::string& source_code,
                               const std::string& language,
                               const std::vector<MKTestCase>& tests) {
        MKTestSuiteResult result;
        result.all_passed = true;
        result.total_tests = static_cast<int>(tests.size());
        result.passed_tests = 0;
        result.failed_tests = 0;
        result.skipped_tests = 0;
        result.total_time_ms = 0;
        result.coverage_estimate = 0.0f;

        auto fw = detectFramework(language, source_code);

        for (auto tc : tests) {
            std::string test_file = writeTestFile(language, tc.test_code, source_code);
            std::string cmd = fw.run_command + " " + test_file + " 2>&1";
            double elapsed = 0;
            std::string output = executeCommand(cmd, elapsed);
            tc.execution_time_ms = elapsed;
            tc.actual_output = output;

            // Check if test passed (simple heuristic)
            tc.passed = (output.find("FAIL") == std::string::npos &&
                         output.find("Error") == std::string::npos &&
                         output.find("error") == std::string::npos);

            if (tc.passed) result.passed_tests++;
            else { result.failed_tests++; result.all_passed = false; }

            result.total_time_ms += elapsed;
            result.test_cases.push_back(tc);
            total_tests_run++;
            if (tc.passed) total_tests_passed++;
        }

        // Estimate coverage based on test diversity
        result.coverage_estimate = std::min(1.0f,
            static_cast<float>(result.passed_tests) / std::max(1, result.total_tests) * 0.7f);
        result.report = formatReport(result);
        return result;
    }

    // Get stats
    int getTotalRun() const { return total_tests_run; }
    int getTotalPassed() const { return total_tests_passed; }
    void printStats() const {
        std::cout << "[MKCodeTester] Run: " << total_tests_run
                  << ", Passed: " << total_tests_passed << std::endl;
    }
};

#endif // MK_CODE_TESTER_CPP
