#ifndef MK_RULE_GENERATOR_CPP
#define MK_RULE_GENERATOR_CPP

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <ctime>

// ============================================================
// MK Rule Generator — Self-Writing Reasoning Rules
// Analyzes knowledge graph patterns and generates new logical
// rules that MK can use for inference. Conservative confidence
// levels (0.5-0.7) for auto-generated rules.
// ============================================================

struct MKProposedRule {
    std::string name;
    std::string condition1;     // "?x relation1 ?y"
    std::string condition2;     // "?y relation2 ?z"
    std::string conclusion;     // "?x derived_rel ?z"
    float confidence;
    int supportCount;           // How many times the pattern appeared
    std::string generatedDate;
    bool approved;
};

class MKRuleGenerator {
private:
    std::vector<MKProposedRule> proposedRules;
    std::unordered_set<std::string> generatedRuleNames;
    std::string rulesFilePath;
    int totalGenerated;
    int totalApplied;
    int minPatternCount;        // Minimum times pattern must appear (default 3)

    // Format a date string
    std::string formatDate(std::time_t t) {
        char buf[32];
        struct tm* tm_info = std::localtime(&t);
        if (tm_info) {
            std::strftime(buf, sizeof(buf), "%Y-%m-%d", tm_info);
            return std::string(buf);
        }
        return "unknown";
    }

    // Generate a unique rule name from two relations
    std::string makeRuleName(const std::string& rel1, const std::string& rel2) {
        return "auto_" + rel1 + "_to_" + rel2;
    }

    // Check if a rule with this name already exists
    bool ruleExists(const std::string& name) {
        return generatedRuleNames.find(name) != generatedRuleNames.end();
    }

    // Count co-occurrence pattern: how many times A's target is B's source
    int countTransitivePattern(const MKPatternGraph& graph,
                              const std::string& rel1,
                              const std::string& rel2) {
        int count = 0;
        const auto& allEdges = graph.getAllEdges();

        // Build a set of targets for rel1
        std::unordered_set<std::string> rel1Targets;
        for (const auto& edge : allEdges) {
            if (edge.relation == rel1) {
                rel1Targets.insert(edge.target);
            }
        }

        // Count how many sources of rel2 are targets of rel1
        for (const auto& edge : allEdges) {
            if (edge.relation == rel2 &&
                rel1Targets.find(edge.source) != rel1Targets.end()) {
                count++;
            }
        }
        return count;
    }

public:
    MKRuleGenerator(const std::string& rulesPath =
                    "ai_core/hre/knowledge_files/auto_rules.mk",
                    int minPattern = 3)
        : rulesFilePath(rulesPath), totalGenerated(0), totalApplied(0),
          minPatternCount(minPattern) {
        loadExistingRules();
        std::cout << "[RULES] Rule generator initialized. "
                  << "Min pattern count: " << minPatternCount
                  << " | Existing auto-rules: "
                  << generatedRuleNames.size() << "\n";
    }

    // Load existing auto-generated rule names to avoid duplicates
    void loadExistingRules() {
        std::ifstream file(rulesFilePath);
        if (!file.is_open()) return;
        std::string line;
        while (std::getline(file, line)) {
            if (line.substr(0, 5) == "RULE|") {
                // Parse: RULE|name|cond1|cond2|conclusion|confidence
                std::stringstream ss(line);
                std::string token, name;
                std::getline(ss, token, '|'); // "RULE"
                std::getline(ss, name, '|');  // name
                if (!name.empty()) {
                    generatedRuleNames.insert(name);
                }
            }
        }
        file.close();
    }

    // Analyze graph patterns and find potential new rules
    std::vector<MKProposedRule> analyzePatterns(
            const MKPatternGraph& graph,
            const std::vector<MKKnowledgeGap>& gaps) {
        std::vector<MKProposedRule> discovered;
        const auto& allEdges = graph.getAllEdges();

        // Step 1: Collect all unique relations in the graph
        std::unordered_set<std::string> relations;
        for (const auto& edge : allEdges) {
            relations.insert(edge.relation);
        }

        // Step 2: For each pair of relations, check transitive pattern
        std::vector<std::string> relList(relations.begin(), relations.end());
        for (size_t i = 0; i < relList.size() && i < 50; i++) {
            for (size_t j = 0; j < relList.size() && j < 50; j++) {
                if (i == j) continue;

                const std::string& rel1 = relList[i];
                const std::string& rel2 = relList[j];
                std::string ruleName = makeRuleName(rel1, rel2);

                // Skip if already generated
                if (ruleExists(ruleName)) continue;

                // Count how many times the pattern appears
                int patternCount = countTransitivePattern(graph, rel1, rel2);

                if (patternCount >= minPatternCount) {
                    MKProposedRule rule;
                    rule.name = ruleName;
                    rule.condition1 = "?x " + rel1 + " ?y";
                    rule.condition2 = "?y " + rel2 + " ?z";
                    rule.conclusion = "?x derived_" + rel1 + "_" + rel2 + " ?z";
                    // Conservative confidence: scale from 0.5 to 0.7 based on count
                    rule.confidence = std::min(0.7f,
                        0.5f + (float)(patternCount - minPatternCount) * 0.02f);
                    rule.supportCount = patternCount;
                    rule.generatedDate = formatDate(std::time(nullptr));
                    rule.approved = false;
                    discovered.push_back(rule);
                }
            }
        }

        // Step 3: Also look at gaps for hints about missing relations
        for (const auto& gap : gaps) {
            if (gap.resolved) continue;
            // Gaps hint at relations the graph might benefit from
            // (used for future priority, not direct rule generation)
        }

        return discovered;
    }

    // Generate a single rule from a detected pattern
    MKProposedRule generateRule(const std::string& rel1,
                               const std::string& rel2,
                               int supportCount) {
        MKProposedRule rule;
        rule.name = makeRuleName(rel1, rel2);
        rule.condition1 = "?x " + rel1 + " ?y";
        rule.condition2 = "?y " + rel2 + " ?z";
        rule.conclusion = "?x derived_" + rel1 + "_" + rel2 + " ?z";
        rule.confidence = std::min(0.7f,
            0.5f + (float)(supportCount - minPatternCount) * 0.02f);
        rule.supportCount = supportCount;
        rule.generatedDate = formatDate(std::time(nullptr));
        rule.approved = false;
        totalGenerated++;
        return rule;
    }

    // Propose up to N rules without applying them
    std::vector<MKProposedRule> proposeRules(
            const MKPatternGraph& graph,
            const std::vector<MKKnowledgeGap>& gaps,
            int maxRules = 5) {
        auto candidates = analyzePatterns(graph, gaps);

        // Sort by support count (most evidence first)
        std::sort(candidates.begin(), candidates.end(),
            [](const MKProposedRule& a, const MKProposedRule& b) {
                return a.supportCount > b.supportCount;
            });

        // Limit to maxRules
        if ((int)candidates.size() > maxRules) {
            candidates.resize(maxRules);
        }

        proposedRules = candidates;
        return candidates;
    }

    // Apply approved rules to the reasoning chains
    int applyApprovedRules(MKReasoningChains& chains,
                           MKPatternGraph& graph) {
        int applied = 0;
        for (auto& rule : proposedRules) {
            if (!rule.approved) continue;
            if (ruleExists(rule.name)) continue;

            // Parse condition strings into chain format
            // condition1: "?x rel1 ?y" -> {?x, rel1, ?y}
            // condition2: "?y rel2 ?z" -> {?y, rel2, ?z}
            // conclusion: "?x derived ?z" -> {?x, derived, ?z}
            std::stringstream ss1(rule.condition1);
            std::string s1, r1, o1;
            ss1 >> s1 >> r1 >> o1;

            std::stringstream ss2(rule.condition2);
            std::string s2, r2, o2;
            ss2 >> s2 >> r2 >> o2;

            std::stringstream ssc(rule.conclusion);
            std::string sc, rc, oc;
            ssc >> sc >> rc >> oc;

            chains.addRule(rule.name,
                          {s1, r1, o1}, {s2, r2, o2},
                          {sc, rc, oc}, rule.confidence);

            generatedRuleNames.insert(rule.name);
            applied++;
            totalApplied++;

            std::cout << "  " << Color::BGREEN << "\u2713" << Color::RESET
                      << " Rule applied: " << Color::BOLD << rule.name
                      << Color::RESET << " (confidence: "
                      << (int)(rule.confidence * 100) << "%)\n";
        }
        return applied;
    }

    // Auto-generate and apply rules (for nightly maintenance)
    int autoGenerateAndApply(MKReasoningChains& chains,
                            MKPatternGraph& graph,
                            const std::vector<MKKnowledgeGap>& gaps) {
        std::cout << "\n  " << Color::BOLD << Color::BMAGENTA
                  << "\u2699 Rule Generation" << Color::RESET << "\n";

        auto candidates = proposeRules(graph, gaps, 10);

        if (candidates.empty()) {
            std::cout << "  " << Color::DIM
                      << "No new patterns found for rule generation."
                      << Color::RESET << "\n";
            return 0;
        }

        std::cout << "  " << Color::BCYAN << "\u25CF" << Color::RESET
                  << " Found " << candidates.size()
                  << " potential new rules.\n";

        // Auto-approve all (nightly mode = automatic)
        for (auto& rule : proposedRules) {
            rule.approved = true;
        }

        int applied = applyApprovedRules(chains, graph);

        // Persist to file
        if (applied > 0) {
            saveGeneratedRules(rulesFilePath);
        }

        std::cout << "  " << Color::BGREEN << "\u2713" << Color::RESET
                  << " Applied " << applied << " new rules.\n";
        return applied;
    }

    // Save generated rules to file
    void saveGeneratedRules(const std::string& filename) {
        std::ofstream file(filename, std::ios::app);
        if (!file.is_open()) {
            std::cout << "  " << Color::RED << "\u2717" << Color::RESET
                      << " Could not write to: " << filename << "\n";
            return;
        }

        for (const auto& rule : proposedRules) {
            if (!rule.approved) continue;
            file << "# Auto-generated rule: " << rule.name << "\n";
            file << "# Confidence: " << rule.confidence << "\n";
            file << "# Generated: " << rule.generatedDate << "\n";
            file << "# Support: " << rule.supportCount << " patterns\n";
            file << "RULE|" << rule.name << "|"
                 << rule.condition1 << "|"
                 << rule.condition2 << "|"
                 << rule.conclusion << "|"
                 << rule.confidence << "\n\n";
        }
        file.close();
    }

    int getTotalGenerated() const { return totalGenerated; }
    int getTotalApplied() const { return totalApplied; }

    void printStats() const {
        std::cout << "  " << Color::BOLD << Color::BMAGENTA
                  << "  Rule Generator:" << Color::RESET << "\n";
        std::cout << "    Total generated: " << totalGenerated << "\n";
        std::cout << "    Total applied: " << totalApplied << "\n";
        std::cout << "    Known rules: " << generatedRuleNames.size() << "\n";
        std::cout << "    Min pattern threshold: " << minPatternCount << "\n";
    }
};

#endif // MK_RULE_GENERATOR_CPP
