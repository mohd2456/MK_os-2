#ifndef MK_CORRECTION_ENGINE_CPP
#define MK_CORRECTION_ENGINE_CPP

#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <regex>

// ============================================================
// MK Correction Engine
// Detects when users correct MK's knowledge and applies fixes
// to the knowledge graph. Supports patterns like:
//   "no, X is actually Y"
//   "wrong, it's not X it's Y"
//   "actually, the capital of France is Paris"
//   "correct: python was created in 1991"
// ============================================================

struct MKCorrection {
    std::string oldFact;   // What was wrong
    std::string newFact;   // What is correct
    std::string source;
    std::string relation;
    std::string target;
    bool valid;
};

class MKCorrectionEngine {
private:
    int correctionsApplied;
    int correctionsDetected;

    std::string toLower(const std::string& s) const {
        std::string result = s;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

    std::string trim(const std::string& s) const {
        size_t start = s.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    }

public:
    MKCorrectionEngine() : correctionsApplied(0), correctionsDetected(0) {}

    // Detect if the input is a correction attempt
    bool detectCorrection(const std::string& input) const {
        std::string lower = toLower(input);

        // Check for correction keywords at the start
        if (lower.substr(0, 3) == "no," || lower.substr(0, 3) == "no ") {
            if (lower.find("is") != std::string::npos ||
                lower.find("was") != std::string::npos ||
                lower.find("are") != std::string::npos) {
                return true;
            }
        }

        // "wrong" at start
        if (lower.find("wrong") == 0 || lower.find("that's wrong") != std::string::npos) {
            return true;
        }

        // "actually" pattern
        if (lower.find("actually") != std::string::npos &&
            (lower.find("is") != std::string::npos ||
             lower.find("was") != std::string::npos ||
             lower.find("are") != std::string::npos)) {
            return true;
        }

        // "it's not X it's Y" pattern
        if (lower.find("it's not") != std::string::npos ||
            lower.find("its not") != std::string::npos ||
            lower.find("it is not") != std::string::npos) {
            if (lower.find("it's") != std::string::npos || lower.find("it is") != std::string::npos) {
                return true;
            }
        }

        // "correct:" prefix
        if (lower.substr(0, 8) == "correct:" || lower.substr(0, 10) == "correction:") {
            return true;
        }

        // "not X, it's Y" or "not X but Y"
        if (lower.find("not ") != std::string::npos &&
            (lower.find(", it's ") != std::string::npos ||
             lower.find(" but ") != std::string::npos)) {
            return true;
        }

        return false;
    }

    // Parse the correction and apply it to the graph
    MKCorrection parseCorrection(const std::string& input) const {
        MKCorrection correction;
        correction.valid = false;
        std::string lower = toLower(input);

        // Pattern: "correct: source|relation|target"
        size_t colonPos = lower.find("correct:");
        if (colonPos == std::string::npos) colonPos = lower.find("correction:");
        if (colonPos != std::string::npos) {
            size_t start = input.find(':', colonPos) + 1;
            std::string factStr = trim(input.substr(start));
            // Try pipe-delimited format
            std::stringstream ss(factStr);
            std::string s, r, t;
            if (std::getline(ss, s, '|') && std::getline(ss, r, '|') && std::getline(ss, t, '|')) {
                correction.source = trim(s);
                correction.relation = trim(r);
                correction.target = trim(t);
                correction.newFact = factStr;
                correction.valid = true;
                return correction;
            }
        }

        // Pattern: "actually, X is Y" or "no, X is Y"
        // Remove prefix words
        std::string cleaned = lower;
        std::vector<std::string> prefixes = {"no,", "no ", "wrong,", "wrong ", "actually,", "actually "};
        for (const auto& prefix : prefixes) {
            if (cleaned.find(prefix) == 0) {
                cleaned = cleaned.substr(prefix.size());
                break;
            }
        }
        cleaned = trim(cleaned);

        // Try to parse "X is Y" or "X was Y"
        std::vector<std::string> connectors = {" is ", " was ", " are ", " were "};
        for (const auto& conn : connectors) {
            size_t pos = cleaned.find(conn);
            if (pos != std::string::npos) {
                correction.source = trim(cleaned.substr(0, pos));
                correction.target = trim(cleaned.substr(pos + conn.size()));
                // Determine relation from connector
                if (conn == " is " || conn == " are ") {
                    correction.relation = "is_a";
                } else {
                    correction.relation = "was";
                }
                correction.newFact = correction.source + "|" + correction.relation + "|" + correction.target;
                correction.valid = !correction.source.empty() && !correction.target.empty();
                return correction;
            }
        }

        return correction;
    }

    // Apply correction to the graph - returns response string
    std::string applyCorrection(MKPatternGraph& graph, const std::string& input) {
        correctionsDetected++;
        MKCorrection correction = parseCorrection(input);

        if (!correction.valid) {
            return "I detected a correction but couldn't parse it. "
                   "Try: \"correct: source|relation|target\" format.";
        }

        // Remove old facts about this source+relation if they exist
        // (The graph may have conflicting info)
        auto existing = graph.getAll(correction.source);
        bool removedOld = false;
        for (const auto& edge : existing) {
            if (edge.relation == correction.relation && edge.target != correction.target) {
                graph.removeEdge(edge.source, edge.relation, edge.target);
                removedOld = true;
            }
        }

        // Add the corrected fact
        graph.persistNewFact(correction.source, correction.relation, correction.target, 1.0f);
        correctionsApplied++;

        std::string response = "Got it! I've updated my knowledge: " +
                               correction.source + " " + correction.relation + " " + correction.target + ".";
        if (removedOld) {
            response += " (Removed conflicting old fact.)";
        }
        return response;
    }

    int getCorrectionsApplied() const { return correctionsApplied; }
    int getCorrectionsDetected() const { return correctionsDetected; }
};

#endif // MK_CORRECTION_ENGINE_CPP
