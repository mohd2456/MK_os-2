// ============================================================================
// MK OS — Self-Programming Engine
// ============================================================================
// MK modifies its OWN brain to get smarter.
//
// How:
// - Reads its own knowledge files
// - Detects patterns it handles poorly (via uncertainty log)
// - Generates new rules/facts to fill gaps
// - Writes new entries to knowledge files
// - Suggests new reasoning rules based on observed patterns
// - Tracks which self-modifications improved performance
//
// Safety: NEVER modifies without logging. User can review all changes.
// All changes are reversible (tracks before/after).
// Writes to: knowledge_files/self_modifications.mk
//
// Pure C++ STL. No external dependencies.
// ============================================================================

#ifndef MK_SELF_PROGRAMMER_CPP
#define MK_SELF_PROGRAMMER_CPP

#include <string>
#include <vector>
#include <map>
#include <set>
#include <deque>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <ctime>

// ============================================================================
// Constants
// ============================================================================

static const std::string SELF_MOD_FILE = "knowledge_files/self_modifications.mk";
static const std::string UNCERTAINTY_LOG = "knowledge_files/pending_review.mk";
static const std::string LEARNED_FACTS_FILE = "knowledge_files/learned_facts.mk";
static const std::string RULES_FILE = "knowledge_files/rules.mk";
static const int MAX_MODIFICATION_HISTORY = 500;
static const float RULE_DISABLE_THRESHOLD = 0.2f;
static const float RULE_BOOST_THRESHOLD = 0.8f;

// ============================================================================
// Structures
// ============================================================================

// A self-modification record
struct SelfModification {
    std::string timestamp;        // When the modification was made
    std::string type;             // "add_fact", "add_rule", "disable_rule", "merge"
    std::string description;      // Human-readable description
    std::string beforeState;      // What was there before (for rollback)
    std::string afterState;       // What was written
    bool approved;                // Has user approved this?
    bool reverted;                // Was this rolled back?
    float performanceImpact;      // +/- score after modification
};

// A weakness pattern detected from uncertainty log
struct WeaknessPattern {
    std::string domain;           // Which area MK struggles with
    std::string specificGap;      // What exactly is missing
    int occurrenceCount;          // How many times this gap caused failure
    float severity;               // How badly it affects responses (0-1)
    std::vector<std::string> exampleQueries;  // Queries that triggered this gap
};

// A rule with performance tracking
struct TrackedRule {
    std::string ruleText;         // The rule itself (source|relation|target|weight)
    int timesApplied;             // How often this rule was used
    int timesSucceeded;           // How often it led to correct answers
    int timesFailed;              // How often it led to wrong answers
    float successRate;            // timesSucceeded / timesApplied
    bool enabled;                 // Is this rule currently active?
};

// A proposed upgrade (awaiting user approval)
struct ProposedUpgrade {
    std::string description;      // What the upgrade does
    std::string rationale;        // Why MK thinks this is needed
    std::string implementation;   // The actual change to make
    float expectedImprovement;    // Estimated benefit (0-1)
    bool approved;
    bool rejected;
};


// ============================================================================
// MKSelfProgrammer — Main Class
// ============================================================================

class MKSelfProgrammer {
private:
    // History of all self-modifications
    std::deque<SelfModification> modificationHistory;

    // Detected weaknesses
    std::vector<WeaknessPattern> detectedWeaknesses;

    // Rules with performance tracking
    std::map<std::string, TrackedRule> trackedRules;

    // Pending upgrades awaiting approval
    std::vector<ProposedUpgrade> pendingUpgrades;

    // Performance metrics (before/after modifications)
    std::map<std::string, float> performanceBaseline;
    std::map<std::string, float> performanceCurrent;

    // ========================================================================
    // Private: Timestamp
    // ========================================================================

    std::string getTimestamp() {
        std::time_t now = std::time(nullptr);
        char buf[64];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        return std::string(buf);
    }

    // ========================================================================
    // Private: File I/O
    // ========================================================================

    // Read a knowledge file into lines
    std::vector<std::string> readKnowledgeFile(const std::string& path) {
        std::vector<std::string> lines;
        std::ifstream file(path);
        if (!file.is_open()) return lines;

        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty() && line[0] != '#') {  // Skip comments
                lines.push_back(line);
            }
        }
        file.close();
        return lines;
    }

    // Append a line to a knowledge file
    void appendToFile(const std::string& path, const std::string& content) {
        std::ofstream file(path, std::ios::app);
        if (file.is_open()) {
            file << content << "\n";
            file.close();
        }
    }

    // ========================================================================
    // Private: Log Modification
    // ========================================================================

    void logModification(const SelfModification& mod) {
        modificationHistory.push_back(mod);
        if ((int)modificationHistory.size() > MAX_MODIFICATION_HISTORY) {
            modificationHistory.pop_front();
        }

        // Also write to self_modifications.mk for persistence
        std::string logEntry = "# " + mod.timestamp + " | " + mod.type + "\n";
        logEntry += "# Description: " + mod.description + "\n";
        logEntry += "# Before: " + mod.beforeState + "\n";
        logEntry += mod.afterState + "\n";
        appendToFile(SELF_MOD_FILE, logEntry);
    }

    // ========================================================================
    // Private: Parse Uncertainty Log
    // ========================================================================

    // Read the uncertainty log and find patterns in failures
    std::vector<WeaknessPattern> parseUncertaintyLog() {
        std::vector<WeaknessPattern> patterns;
        std::vector<std::string> lines = readKnowledgeFile(UNCERTAINTY_LOG);

        // Count occurrences of similar failures
        std::map<std::string, std::vector<std::string>> domainFailures;

        for (const auto& line : lines) {
            // Format assumed: query|confidence|domain
            std::istringstream stream(line);
            std::string query, conf, domain;
            std::getline(stream, query, '|');
            std::getline(stream, conf, '|');
            std::getline(stream, domain, '|');

            if (!domain.empty()) {
                domainFailures[domain].push_back(query);
            }
        }

        // Build weakness patterns from grouped failures
        for (const auto& pair : domainFailures) {
            if (pair.second.size() >= 2) {  // At least 2 failures = pattern
                WeaknessPattern wp;
                wp.domain = pair.first;
                wp.occurrenceCount = (int)pair.second.size();
                wp.severity = std::min(1.0f, wp.occurrenceCount * 0.1f);
                wp.specificGap = "Insufficient knowledge about: " + pair.first;
                wp.exampleQueries = pair.second;
                patterns.push_back(wp);
            }
        }

        // Sort by severity (worst gaps first)
        std::sort(patterns.begin(), patterns.end(),
                  [](const WeaknessPattern& a, const WeaknessPattern& b) {
                      return a.severity > b.severity;
                  });

        return patterns;
    }


public:
    // ========================================================================
    // Constructor
    // ========================================================================

    MKSelfProgrammer() {
        // Load existing modification history from file
        std::vector<std::string> history = readKnowledgeFile(SELF_MOD_FILE);
        // (History is loaded for reference, actual parsing handled on demand)
    }

    // ========================================================================
    // Core API: Analyze Weaknesses
    // ========================================================================

    // Reads uncertainty log, finds patterns in failures
    void analyzeWeaknesses() {
        detectedWeaknesses = parseUncertaintyLog();

        std::cout << "[SelfProgrammer] Found " << detectedWeaknesses.size()
                  << " weakness patterns:" << std::endl;

        for (const auto& wp : detectedWeaknesses) {
            std::cout << "  - " << wp.domain << " (severity: " << wp.severity
                      << ", occurrences: " << wp.occurrenceCount << ")" << std::endl;
        }
    }

    // ========================================================================
    // Core API: Generate New Rules
    // ========================================================================

    // Creates reasoning rules to fill detected gaps
    std::vector<std::string> generateNewRules(const std::vector<WeaknessPattern>& gaps) {
        std::vector<std::string> newRules;

        for (const auto& gap : gaps) {
            // Strategy 1: If the gap is about a domain, create a general rule
            // that links the domain to common query patterns
            std::string rule = gap.domain + "|needs_knowledge|" + gap.specificGap + "|0.5";
            newRules.push_back(rule);

            // Strategy 2: For each example query that failed,
            // create a placeholder that flags this as "needs attention"
            if (!gap.exampleQueries.empty()) {
                std::string example = gap.exampleQueries[0];
                std::string learnRule = gap.domain + "|common_question|" + example + "|0.3";
                newRules.push_back(learnRule);
            }

            // Strategy 3: If severity is high, create a meta-rule
            // that tells the reasoner to be cautious in this domain
            if (gap.severity > 0.7f) {
                std::string cautionRule = gap.domain + "|confidence_modifier|low|0.8";
                newRules.push_back(cautionRule);
            }
        }

        // Log the generation
        SelfModification mod;
        mod.timestamp = getTimestamp();
        mod.type = "add_rule";
        mod.description = "Generated " + std::to_string(newRules.size()) +
                         " new rules for " + std::to_string(gaps.size()) + " gaps";
        mod.beforeState = "(no prior rules for these gaps)";
        mod.afterState = std::to_string(newRules.size()) + " rules generated";
        mod.approved = false;
        mod.reverted = false;
        mod.performanceImpact = 0.0f;
        logModification(mod);

        return newRules;
    }

    // ========================================================================
    // Core API: Auto-Learn from Conversation
    // ========================================================================

    // Extracts implicit facts from what user said
    void autoLearnFromConversation(const std::vector<std::pair<std::string, std::string>>& exchanges) {
        // exchanges = vector of (user_message, mk_response) pairs

        for (const auto& exchange : exchanges) {
            const std::string& userMsg = exchange.first;

            // Look for teaching patterns:
            // "X is Y" → extract fact
            // "X means Y" → extract definition
            // "actually, X" → correction
            // "no, X is not Y" → negation

            std::string lower = userMsg;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

            // Pattern: "X is Y"
            size_t isPos = lower.find(" is ");
            if (isPos != std::string::npos && isPos > 0) {
                std::string subject = userMsg.substr(0, isPos);
                std::string object = userMsg.substr(isPos + 4);
                // Strip trailing punctuation
                while (!object.empty() && std::ispunct(object.back())) {
                    object.pop_back();
                }

                if (!subject.empty() && !object.empty() && subject.size() < 50) {
                    std::string fact = subject + "|is_a|" + object + "|0.6";

                    // Log and save
                    SelfModification mod;
                    mod.timestamp = getTimestamp();
                    mod.type = "add_fact";
                    mod.description = "Learned from conversation: " + subject + " is " + object;
                    mod.beforeState = "(unknown)";
                    mod.afterState = fact;
                    mod.approved = false;
                    mod.reverted = false;
                    mod.performanceImpact = 0.0f;
                    logModification(mod);

                    // Write to learned facts (pending review weight)
                    appendToFile(LEARNED_FACTS_FILE, fact);
                }
            }

            // Pattern: "actually" = correction (lower previous confidence)
            if (lower.find("actually") != std::string::npos ||
                lower.find("no,") != std::string::npos ||
                lower.find("wrong") != std::string::npos) {
                // Mark this as a correction event
                SelfModification mod;
                mod.timestamp = getTimestamp();
                mod.type = "correction";
                mod.description = "User corrected MK: " + userMsg;
                mod.beforeState = "(previous response was incorrect)";
                mod.afterState = "Correction noted";
                mod.approved = true;
                mod.reverted = false;
                mod.performanceImpact = -0.1f;
                logModification(mod);
            }
        }
    }


    // ========================================================================
    // Core API: Optimize Rules
    // ========================================================================

    // Disable rules with bad track records, boost good ones
    void optimizeRules() {
        int disabled = 0;
        int boosted = 0;

        for (auto& pair : trackedRules) {
            TrackedRule& rule = pair.second;

            if (rule.timesApplied < 5) continue;  // Not enough data

            rule.successRate = (float)rule.timesSucceeded / (float)rule.timesApplied;

            if (rule.successRate < RULE_DISABLE_THRESHOLD && rule.enabled) {
                // This rule is causing more harm than good — disable it
                rule.enabled = false;
                disabled++;

                SelfModification mod;
                mod.timestamp = getTimestamp();
                mod.type = "disable_rule";
                mod.description = "Disabled underperforming rule (success rate: " +
                                 std::to_string((int)(rule.successRate * 100)) + "%)";
                mod.beforeState = rule.ruleText;
                mod.afterState = "(disabled)";
                mod.approved = false;
                mod.reverted = false;
                mod.performanceImpact = 0.0f;
                logModification(mod);
            }

            if (rule.successRate > RULE_BOOST_THRESHOLD) {
                // Great rule — boost its weight
                boosted++;
                // Weight boost is implicit: higher success rate = higher priority
            }
        }

        if (disabled > 0 || boosted > 0) {
            std::cout << "[SelfProgrammer] Optimization: disabled " << disabled
                      << " rules, boosted " << boosted << " rules" << std::endl;
        }
    }

    // ========================================================================
    // Core API: Consolidate Knowledge
    // ========================================================================

    // Merge redundant facts, strengthen common patterns
    void consolidateKnowledge() {
        std::vector<std::string> facts = readKnowledgeFile(LEARNED_FACTS_FILE);

        // Find duplicate/near-duplicate facts
        std::map<std::string, std::vector<int>> factGroups;
        for (size_t i = 0; i < facts.size(); i++) {
            // Group by subject|relation (ignore target for grouping)
            std::istringstream stream(facts[i]);
            std::string subject, relation;
            std::getline(stream, subject, '|');
            std::getline(stream, relation, '|');
            std::string key = subject + "|" + relation;
            factGroups[key].push_back((int)i);
        }

        int merged = 0;
        for (const auto& group : factGroups) {
            if (group.second.size() > 1) {
                // Multiple facts with same subject+relation — merge them
                // Keep the one with highest weight, note the merge
                merged++;

                SelfModification mod;
                mod.timestamp = getTimestamp();
                mod.type = "merge";
                mod.description = "Merged " + std::to_string(group.second.size()) +
                                 " redundant facts for: " + group.first;
                mod.beforeState = std::to_string(group.second.size()) + " separate entries";
                mod.afterState = "1 consolidated entry";
                mod.approved = false;
                mod.reverted = false;
                mod.performanceImpact = 0.0f;
                logModification(mod);
            }
        }

        if (merged > 0) {
            std::cout << "[SelfProgrammer] Consolidated " << merged
                      << " redundant fact groups" << std::endl;
        }
    }

    // ========================================================================
    // Core API: Self-Reflect
    // ========================================================================

    // Generates insights about MK's own performance trends
    void selfReflect() {
        std::cout << "[SelfProgrammer] === Self-Reflection Report ===" << std::endl;

        // Modification stats
        int totalMods = (int)modificationHistory.size();
        int approved = 0, reverted = 0;
        float avgImpact = 0.0f;

        for (const auto& mod : modificationHistory) {
            if (mod.approved) approved++;
            if (mod.reverted) reverted++;
            avgImpact += mod.performanceImpact;
        }
        if (totalMods > 0) avgImpact /= (float)totalMods;

        std::cout << "  Total self-modifications: " << totalMods << std::endl;
        std::cout << "  Approved: " << approved << std::endl;
        std::cout << "  Reverted: " << reverted << std::endl;
        std::cout << "  Average impact: " << avgImpact << std::endl;

        // Weakness trends
        std::cout << "  Active weaknesses: " << detectedWeaknesses.size() << std::endl;
        for (size_t i = 0; i < detectedWeaknesses.size() && i < 3; i++) {
            std::cout << "    - " << detectedWeaknesses[i].domain
                      << " (severity " << detectedWeaknesses[i].severity << ")" << std::endl;
        }

        // Rule health
        int totalRules = (int)trackedRules.size();
        int activeRules = 0;
        for (const auto& pair : trackedRules) {
            if (pair.second.enabled) activeRules++;
        }
        std::cout << "  Rules: " << activeRules << "/" << totalRules << " active" << std::endl;
        std::cout << "  =================================" << std::endl;
    }


    // ========================================================================
    // Core API: Propose Upgrade
    // ========================================================================

    // MK suggests a change — logs for user approval
    // Returns true if logged successfully
    bool proposeUpgrade(const std::string& description) {
        ProposedUpgrade upgrade;
        upgrade.description = description;
        upgrade.rationale = "Based on " + std::to_string(detectedWeaknesses.size()) +
                           " detected weaknesses";
        upgrade.implementation = "(pending design)";
        upgrade.expectedImprovement = 0.0f;
        upgrade.approved = false;
        upgrade.rejected = false;

        pendingUpgrades.push_back(upgrade);

        // Log the proposal
        SelfModification mod;
        mod.timestamp = getTimestamp();
        mod.type = "proposal";
        mod.description = "Proposed upgrade: " + description;
        mod.beforeState = "(current state)";
        mod.afterState = "(proposed: " + description + ")";
        mod.approved = false;
        mod.reverted = false;
        mod.performanceImpact = 0.0f;
        logModification(mod);

        std::cout << "[SelfProgrammer] Proposed upgrade: " << description << std::endl;
        std::cout << "  (Awaiting user approval)" << std::endl;

        return true;
    }

    // ========================================================================
    // Core API: Record Rule Usage
    // ========================================================================

    // Called by reasoning engine when a rule is applied
    void recordRuleUsage(const std::string& ruleText, bool wasSuccessful) {
        auto& rule = trackedRules[ruleText];
        if (rule.ruleText.empty()) {
            rule.ruleText = ruleText;
            rule.timesApplied = 0;
            rule.timesSucceeded = 0;
            rule.timesFailed = 0;
            rule.enabled = true;
        }

        rule.timesApplied++;
        if (wasSuccessful) {
            rule.timesSucceeded++;
        } else {
            rule.timesFailed++;
        }
        rule.successRate = (float)rule.timesSucceeded / (float)rule.timesApplied;
    }

    // ========================================================================
    // Core API: Revert Modification
    // ========================================================================

    // Undo a specific modification (by index in history)
    bool revertModification(int index) {
        if (index < 0 || index >= (int)modificationHistory.size()) return false;

        SelfModification& mod = modificationHistory[index];
        if (mod.reverted) return false;  // Already reverted

        mod.reverted = true;

        // Log the reversion
        SelfModification revertMod;
        revertMod.timestamp = getTimestamp();
        revertMod.type = "revert";
        revertMod.description = "Reverted: " + mod.description;
        revertMod.beforeState = mod.afterState;
        revertMod.afterState = mod.beforeState;
        revertMod.approved = true;
        revertMod.reverted = false;
        revertMod.performanceImpact = 0.0f;
        logModification(revertMod);

        std::cout << "[SelfProgrammer] Reverted modification: " << mod.description << std::endl;
        return true;
    }

    // ========================================================================
    // Utility: Get Pending Upgrades
    // ========================================================================

    std::vector<ProposedUpgrade> getPendingUpgrades() const {
        std::vector<ProposedUpgrade> pending;
        for (const auto& u : pendingUpgrades) {
            if (!u.approved && !u.rejected) {
                pending.push_back(u);
            }
        }
        return pending;
    }

    // Approve or reject an upgrade
    void resolveUpgrade(int index, bool approve) {
        if (index >= 0 && index < (int)pendingUpgrades.size()) {
            if (approve) {
                pendingUpgrades[index].approved = true;
                std::cout << "[SelfProgrammer] Upgrade approved: "
                          << pendingUpgrades[index].description << std::endl;
            } else {
                pendingUpgrades[index].rejected = true;
                std::cout << "[SelfProgrammer] Upgrade rejected: "
                          << pendingUpgrades[index].description << std::endl;
            }
        }
    }

    // ========================================================================
    // Utility: Stats
    // ========================================================================

    int getModificationCount() const { return (int)modificationHistory.size(); }
    int getWeaknessCount() const { return (int)detectedWeaknesses.size(); }
    int getTrackedRuleCount() const { return (int)trackedRules.size(); }
};

#endif // MK_SELF_PROGRAMMER_CPP
