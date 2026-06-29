#ifndef MK_PROMETHEUS_BIRTH_CHAMBER_CPP
#define MK_PROMETHEUS_BIRTH_CHAMBER_CPP

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <cstdio>
#include <cmath>

// ===================================================================================
// MK PROMETHEUS — BIRTH CHAMBER
// ===================================================================================
// Where new code is EVOLVED through tournament selection.
// Takes goal droplets (what to build) + code droplets (how to build it),
// assembles candidates, compiles/tests them, and returns the winner.
// ===================================================================================

struct MKCodeCandidate {
    std::string code;
    std::string language;       // "python", "cpp", "bash"
    bool compiled;
    bool passed_tests;
    float execution_time;       // seconds
    std::string output;
    float score;                // tournament score

    MKCodeCandidate()
        : compiled(false), passed_tests(false), execution_time(0.0f), score(0.0f) {}
};

class MKBirthChamber {
private:
    std::mt19937 rng_;
    std::string tempDir_;
    int candidatesGenerated_ = 0;
    int tournamentsRun_ = 0;
    int winnersProduced_ = 0;

    long long now() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    // Detect language from code content
    std::string detectLanguage(const std::string& code) const {
        if (code.find("def ") != std::string::npos || code.find("import ") != std::string::npos ||
            code.find("print(") != std::string::npos) {
            return "python";
        }
        if (code.find("#include") != std::string::npos || code.find("int main") != std::string::npos ||
            code.find("std::") != std::string::npos) {
            return "cpp";
        }
        if (code.find("#!/bin/bash") != std::string::npos || code.find("echo ") != std::string::npos ||
            code.find("if [") != std::string::npos) {
            return "bash";
        }
        return "python";  // Default
    }

    // Get file extension for language
    std::string getExtension(const std::string& lang) const {
        if (lang == "python") return ".py";
        if (lang == "cpp") return ".cpp";
        if (lang == "bash") return ".sh";
        return ".py";
    }

    // Get compile command for language
    std::string getCompileCmd(const std::string& lang, const std::string& srcPath,
                              const std::string& outPath) const {
        if (lang == "cpp") {
            return "g++ -o " + outPath + " " + srcPath + " -std=c++17 2>/dev/null";
        }
        if (lang == "python") {
            return "python3 -c \"import py_compile; py_compile.compile('" + srcPath + "', doraise=True)\" 2>/dev/null";
        }
        if (lang == "bash") {
            return "bash -n " + srcPath + " 2>/dev/null";
        }
        return "";
    }

    // Escape a string for safe use inside single quotes in shell commands.
    // Replaces every ' with '\'' (end quote, escaped quote, restart quote).
    std::string shellEscape(const std::string& s) const {
        std::string escaped;
        escaped.reserve(s.size() + 16);
        for (char c : s) {
            if (c == '\'') {
                escaped += "'\\''";
            } else {
                escaped += c;
            }
        }
        return escaped;
    }

    // Get run command for language
    std::string getRunCmd(const std::string& lang, const std::string& srcPath,
                          const std::string& outPath, const std::string& input) const {
        std::string safeInput = shellEscape(input);
        if (lang == "cpp") {
            return "echo '" + safeInput + "' | timeout 5 " + outPath + " 2>/dev/null";
        }
        if (lang == "python") {
            return "echo '" + safeInput + "' | timeout 5 python3 " + srcPath + " 2>/dev/null";
        }
        if (lang == "bash") {
            return "echo '" + safeInput + "' | timeout 5 bash " + srcPath + " 2>/dev/null";
        }
        return "";
    }

    // Execute a command and capture output
    std::string execCapture(const std::string& cmd) const {
        std::string result;
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) return "";
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        int status = pclose(pipe);
        (void)status;
        return result;
    }

    // Combine code fragments into a candidate
    std::string assembleCandidate(const std::vector<std::string>& fragments,
                                   const std::string& goal, int variation) {
        std::string code;

        if (fragments.empty()) return "";

        // Different assembly strategies based on variation number
        switch (variation % 5) {
            case 0:  // Sequential: all fragments in order
                code += "# Goal: " + goal.substr(0, 80) + "\n";
                for (const auto& f : fragments) {
                    code += f + "\n";
                }
                break;
            case 1:  // Wrapped in function
                code += "def solve():\n";
                for (const auto& f : fragments) {
                    code += "    " + f + "\n";
                }
                code += "\nresult = solve()\nif result is not None:\n    print(result)\n";
                break;
            case 2:  // With error handling
                code += "try:\n";
                for (const auto& f : fragments) {
                    code += "    " + f + "\n";
                }
                code += "except Exception as e:\n    print(f'Error: {e}')\n";
                break;
            case 3:  // Reversed order (sometimes logic flows better)
                for (int i = (int)fragments.size() - 1; i >= 0; i--) {
                    code += fragments[i] + "\n";
                }
                break;
            case 4:  // With input handling
                code += "import sys\n";
                code += "data = sys.stdin.read().strip() if not sys.stdin.isatty() else ''\n";
                for (const auto& f : fragments) {
                    code += f + "\n";
                }
                break;
        }

        return code;
    }

public:
    MKBirthChamber() : tempDir_("prometheus_data/tmp") {
        auto seed = std::chrono::steady_clock::now().time_since_epoch().count();
        rng_ = std::mt19937(static_cast<unsigned>(seed));
        // Ensure temp directory exists
        std::system(("mkdir -p " + tempDir_).c_str());
        std::cout << "[PROMETHEUS] BirthChamber initialized\n";
    }

    // =======================================================================
    // CONCEIVE — Generate code candidates from goal + code droplets
    // =======================================================================
    std::vector<MKCodeCandidate> conceive(const std::vector<int>& goalDroplets,
                                           const std::vector<int>& codeDropletIds,
                                           MKDropletPool& pool) {
        std::vector<MKCodeCandidate> candidates;

        // Extract goal description from logic droplets
        std::string goalDesc;
        for (int id : goalDroplets) {
            if (id >= 0 && id < pool.size() && pool.get(id).isAlive()) {
                goalDesc += pool.get(id).content + " ";
            }
        }

        // Extract code fragments from high-precision droplets
        std::vector<std::string> fragments;
        for (int id : codeDropletIds) {
            if (id >= 0 && id < pool.size() && pool.get(id).isAlive() && pool.get(id).isCode()) {
                fragments.push_back(pool.get(id).content);
            }
        }

        if (fragments.empty()) return candidates;

        // Generate 5-10 different combinations
        int numCandidates = std::min(8, std::max(5, (int)fragments.size()));

        for (int i = 0; i < numCandidates; i++) {
            MKCodeCandidate candidate;

            // Select subset of fragments for this candidate
            std::vector<std::string> subset;
            int subsetSize = 1 + (rng_() % std::min(4, (int)fragments.size()));
            std::vector<int> indices(fragments.size());
            std::iota(indices.begin(), indices.end(), 0);
            std::shuffle(indices.begin(), indices.end(), rng_);

            for (int j = 0; j < subsetSize && j < (int)indices.size(); j++) {
                subset.push_back(fragments[indices[j]]);
            }

            candidate.code = assembleCandidate(subset, goalDesc, i);
            candidate.language = detectLanguage(candidate.code);
            candidates.push_back(candidate);
            candidatesGenerated_++;
        }

        return candidates;
    }

    // =======================================================================
    // SAFETY: Check if assembled code contains dangerous patterns.
    // Mirrors the blocklist from MKCodeRunner::isDangerous() to prevent
    // the BirthChamber feedback loop from executing harmful code.
    // =======================================================================
    bool containsDangerousPattern(const std::string& code) const {
        std::string lower = code;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        static const std::vector<std::string> patterns = {
            "rm -rf /", "rm -rf /*", "rm -r -f", "rm --recursive",
            "rm -rf ~", "rm -rf $home",
            ":(){ :|:& };:",
            "dd if=/dev/", "mkfs.", "chmod -r 777 /",
            "> /dev/sda", "shutdown", "reboot", "halt",
            "$(rm ", "$(dd ", "$(mkfs", "$(shutdown",
            "`rm ", "`dd ", "`mkfs", "`shutdown",
            "os.system(", "os.popen(", "subprocess.call(",
            "subprocess.run(", "subprocess.popen(",
            "__import__('os')", "__import__(\"os\")",
            "import socket", "import http", "import urllib",
            "require('child_process')", "require(\"child_process\")",
            "child_process", "shutil.rmtree",
            "fs.rmdirsync", "fs.unlinksync",
            "std::filesystem::remove_all"
        };

        for (const auto& p : patterns) {
            if (lower.find(p) != std::string::npos) return true;
        }
        return false;
    }

    // =======================================================================
    // TOURNAMENT — Compile, test, and select the best candidate
    // =======================================================================
    MKCodeCandidate tournament(std::vector<MKCodeCandidate>& candidates,
                                const std::vector<std::pair<std::string, std::string>>& testCases) {
        tournamentsRun_++;

        for (auto& candidate : candidates) {
            if (candidate.code.empty()) continue;

            // Safety gate: skip candidates that contain dangerous patterns.
            // This prevents the absorb() feedback loop from promoting MK's own
            // response text into code that could execute harmful commands.
            if (containsDangerousPattern(candidate.code)) {
                candidate.compiled = false;
                candidate.score = 0.0f;
                continue;
            }

            std::string ext = getExtension(candidate.language);
            std::string srcPath = tempDir_ + "/candidate" + ext;
            std::string outPath = tempDir_ + "/candidate_out";

            // Write source code to temp file
            {
                std::ofstream f(srcPath);
                f << candidate.code;
                f.close();
            }

            // Compile
            std::string compileCmd = getCompileCmd(candidate.language, srcPath, outPath);
            if (!compileCmd.empty()) {
                int compileResult = std::system(compileCmd.c_str());
                candidate.compiled = (compileResult == 0);
            } else {
                candidate.compiled = true;  // Interpreted language, no compile step
            }

            if (!candidate.compiled) {
                candidate.score = 0.0f;
                continue;
            }

            // Run tests
            float correctCount = 0;
            float totalTime = 0.0f;

            if (testCases.empty()) {
                // No test cases: just verify it runs without crashing
                std::string runCmd = getRunCmd(candidate.language, srcPath, outPath, "");
                auto start = std::chrono::steady_clock::now();
                candidate.output = execCapture(runCmd);
                auto end = std::chrono::steady_clock::now();
                totalTime = std::chrono::duration<float>(end - start).count();
                candidate.passed_tests = true;
                correctCount = 1.0f;
            } else {
                for (const auto& [input, expected] : testCases) {
                    std::string runCmd = getRunCmd(candidate.language, srcPath, outPath, input);
                    auto start = std::chrono::steady_clock::now();
                    std::string output = execCapture(runCmd);
                    auto end = std::chrono::steady_clock::now();
                    totalTime += std::chrono::duration<float>(end - start).count();

                    // Trim whitespace for comparison
                    while (!output.empty() && (output.back() == '\n' || output.back() == ' '))
                        output.pop_back();
                    std::string exp = expected;
                    while (!exp.empty() && (exp.back() == '\n' || exp.back() == ' '))
                        exp.pop_back();

                    if (output == exp) correctCount += 1.0f;
                    candidate.output = output;
                }
                candidate.passed_tests = (correctCount == (float)testCases.size());
            }

            candidate.execution_time = totalTime;

            // Score: correctness * speed * brevity
            float correctness = testCases.empty() ? 1.0f : (correctCount / (float)testCases.size());
            float speedScore = std::max(0.0f, 1.0f - totalTime / 5.0f);  // Penalty above 5s
            float brevityScore = std::max(0.1f, 1.0f - (float)candidate.code.size() / 5000.0f);
            candidate.score = correctness * 0.6f + speedScore * 0.2f + brevityScore * 0.2f;

            // Cleanup temp files
            std::remove(srcPath.c_str());
            std::remove(outPath.c_str());
        }

        // Find winner
        auto winner = std::max_element(candidates.begin(), candidates.end(),
            [](const MKCodeCandidate& a, const MKCodeCandidate& b) {
                return a.score < b.score;
            });

        if (winner != candidates.end() && winner->score > 0.0f) {
            winnersProduced_++;
            return *winner;
        }

        // All failed: return empty
        return MKCodeCandidate();
    }

    // =======================================================================
    // GENERATE TEST CASES — Heuristic test case generation from goal
    // =======================================================================
    std::vector<std::pair<std::string, std::string>> generateTestCases(const std::string& goalDesc) {
        std::vector<std::pair<std::string, std::string>> tests;
        std::string lower = goalDesc;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        // Detect type patterns from description and generate appropriate tests
        if (lower.find("sort") != std::string::npos) {
            tests.push_back({"3 1 2", "1 2 3"});
            tests.push_back({"5 3 8 1", "1 3 5 8"});
            tests.push_back({"1", "1"});
        } else if (lower.find("reverse") != std::string::npos) {
            tests.push_back({"hello", "olleh"});
            tests.push_back({"abc", "cba"});
        } else if (lower.find("sum") != std::string::npos || lower.find("add") != std::string::npos) {
            tests.push_back({"1 2 3", "6"});
            tests.push_back({"10 20", "30"});
        } else if (lower.find("count") != std::string::npos) {
            tests.push_back({"hello world", "2"});
            tests.push_back({"one", "1"});
        } else if (lower.find("uppercase") != std::string::npos || lower.find("upper") != std::string::npos) {
            tests.push_back({"hello", "HELLO"});
            tests.push_back({"world", "WORLD"});
        } else if (lower.find("lowercase") != std::string::npos || lower.find("lower") != std::string::npos) {
            tests.push_back({"HELLO", "hello"});
            tests.push_back({"WORLD", "world"});
        } else if (lower.find("fibonacci") != std::string::npos || lower.find("fib") != std::string::npos) {
            tests.push_back({"5", "5"});
            tests.push_back({"10", "55"});
        } else if (lower.find("factorial") != std::string::npos) {
            tests.push_back({"5", "120"});
            tests.push_back({"3", "6"});
        } else if (lower.find("palindrome") != std::string::npos) {
            tests.push_back({"racecar", "true"});
            tests.push_back({"hello", "false"});
        }

        // If no specific tests detected, return empty (code just needs to not crash)
        return tests;
    }

    // =======================================================================
    // STYLE — Apply identity code style preferences
    // =======================================================================
    std::string style(const std::string& code, const MKIdentityState& identity) {
        if (code.empty()) return code;

        std::string styled = code;

        if (identity.code_style == "commented") {
            // Add header comment
            std::string header = "# Generated by MK Prometheus Engine\n";
            header += "# ---\n";
            styled = header + styled;
        } else if (identity.code_style == "verbose") {
            // Add header + section comments
            std::string header = "# ============================================\n";
            header += "# Generated by MK Prometheus Engine\n";
            header += "# Evolved through tournament selection\n";
            header += "# ============================================\n\n";
            styled = header + styled;
        }
        // "minimal" = no changes

        // If user is beginner, add explanation comment
        if (identity.user_skill_level < 0.3f) {
            styled += "\n# How this works:\n";
            styled += "# This code was assembled from proven patterns.\n";
            styled += "# Each line handles one part of the task.\n";
        }

        return styled;
    }

    // Stats
    std::string getStats() const {
        std::ostringstream ss;
        ss << "BirthChamber { candidates=" << candidatesGenerated_
           << ", tournaments=" << tournamentsRun_
           << ", winners=" << winnersProduced_ << " }";
        return ss.str();
    }
};

#endif // MK_PROMETHEUS_BIRTH_CHAMBER_CPP
