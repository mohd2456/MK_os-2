#ifndef MK_REASONING_CHAINS_CPP
#define MK_REASONING_CHAINS_CPP

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <fstream>
#include <sstream>

// ===================================================================================
// MK HYBRID REASONING ENGINE — LAYER 2: REASONING CHAINS
// ===================================================================================
// This is where MK "thinks." NOT neural network inference.
// Instead: logical rules that derive NEW facts from existing graph knowledge.
//
// Example Rule: IF X is_a Y AND Y has Z THEN X has Z
//   → "cat is_a animal" + "animal has legs" → DERIVED: "cat has legs"
//
// Example Rule: IF X lives_in Y AND Y located_in Z THEN X located_in Z
//   → "mohammed lives_in city" + "city located_in country" → DERIVED
//
// Rules load from files. You can add new rules without recompiling.
// ===================================================================================

// Forward declaration (MKPatternGraph included from pattern_graph.cpp)
#include "pattern_graph.cpp"

// A reasoning rule template
struct MKRule {
    int id;
    std::string name;
    
    // Condition pattern: what edges must exist
    // Format: "?x relation1 ?y" AND "?y relation2 ?z"
    struct Condition {
        std::string var_subject;     // Variable name (e.g., "?x")
        std::string relation;        // Fixed relation to match
        std::string var_object;      // Variable name (e.g., "?y")
    };
    std::vector<Condition> conditions;
    
    // Conclusion: what new edge to create if conditions match
    struct Conclusion {
        std::string var_subject;
        std::string relation;
        std::string var_object;
    };
    Conclusion conclusion;
    
    float confidence;    // How confident are derived facts (0.0-1.0)
    bool enabled;
};

// Result of applying reasoning
struct MKInference {
    std::string derived_source;
    std::string derived_relation;
    std::string derived_target;
    float confidence;
    std::string rule_name;          // Which rule produced this
    std::vector<std::string> chain; // The reasoning steps taken
};

class MKReasoningChains {
private:
    std::vector<MKRule> rules;
    std::vector<MKInference> inferences;    // Cache of derived facts
    int nextRuleId;
    int total_inferences;
    std::string rules_dir;

    // Parse a condition string like "?x is_a ?y"
    MKRule::Condition parseCondition(const std::string& text) {
        MKRule::Condition cond;
        std::stringstream ss(text);
        ss >> cond.var_subject >> cond.relation >> cond.var_object;
        return cond;
    }

public:
    MKReasoningChains(const std::string& rulesDirectory = "knowledge_files") 
        : nextRuleId(0), total_inferences(0), rules_dir(rulesDirectory) {
        std::cout << "[REASONING] Logic chain engine initialized.\n";
        loadBuiltinRules();
    }

    // ─────────────────────────────────────────
    //  BUILT-IN REASONING RULES
    // ─────────────────────────────────────────

    void loadBuiltinRules() {
        // Rule 1: Inheritance — if X is_a Y and Y has Z, then X has Z
        addRule("inheritance_has", 
                {"?x", "is_a", "?y"}, {"?y", "has", "?z"},
                {"?x", "has", "?z"}, 0.9f);

        // Rule 2: Inheritance — if X is_a Y and Y can Z, then X can Z
        addRule("inheritance_can",
                {"?x", "is_a", "?y"}, {"?y", "can", "?z"},
                {"?x", "can", "?z"}, 0.85f);

        // Rule 3: Location transitivity — if X lives_in Y and Y located_in Z, then X located_in Z
        addRule("location_transitive",
                {"?x", "lives_in", "?y"}, {"?y", "located_in", "?z"},
                {"?x", "located_in", "?z"}, 0.95f);

        // Rule 4: Ownership — if X owns Y and Y is_a Z, then X has Z
        addRule("ownership_type",
                {"?x", "owns", "?y"}, {"?y", "is_a", "?z"},
                {"?x", "has", "?z"}, 0.8f);

        // Rule 5: If X part_of Y and Y has property Z, X might share it
        addRule("part_shares_property",
                {"?x", "part_of", "?y"}, {"?y", "has_property", "?z"},
                {"?x", "has_property", "?z"}, 0.6f);

        // Rule 6: If X created Y, then Y created_by X (symmetric inference)
        addRule("created_symmetric",
                {"?x", "created", "?y"}, {},
                {"?y", "created_by", "?x"}, 1.0f);

        // Rule 7: If X likes Y and Y is_a Z, then X likes Z (generalization)
        addRule("preference_generalize",
                {"?x", "likes", "?y"}, {"?y", "is_a", "?z"},
                {"?x", "interested_in", "?z"}, 0.6f);

        // Rule 8: If X is_a Y and Y is_a Z, then X is_a Z (transitive class)
        addRule("transitive_isa",
                {"?x", "is_a", "?y"}, {"?y", "is_a", "?z"},
                {"?x", "is_a", "?z"}, 0.85f);

        // Rule 9: If X opposite_of Y and Y has Z, then X lacks Z
        addRule("opposite_negation",
                {"?x", "opposite_of", "?y"}, {"?y", "has", "?z"},
                {"?x", "lacks", "?z"}, 0.7f);

        // Rule 10: If X needs Y and Y requires Z, then X needs Z
        addRule("transitive_needs",
                {"?x", "needs", "?y"}, {"?y", "requires", "?z"},
                {"?x", "needs", "?z"}, 0.75f);

        std::cout << "[REASONING] Loaded " << rules.size() << " reasoning rules.\n";
    }

    // Add a two-condition rule
    void addRule(const std::string& name,
                 MKRule::Condition cond1, MKRule::Condition cond2,
                 MKRule::Conclusion conclusion, float confidence) {
        MKRule rule;
        rule.id = nextRuleId++;
        rule.name = name;
        rule.conditions.push_back(cond1);
        if (!cond2.var_subject.empty()) {
            rule.conditions.push_back(cond2);
        }
        rule.conclusion = conclusion;
        rule.confidence = confidence;
        rule.enabled = true;
        rules.push_back(rule);
    }

    // Convenience overload with struct initialization
    void addRule(const std::string& name,
                 std::initializer_list<std::string> c1,
                 std::initializer_list<std::string> c2,
                 std::initializer_list<std::string> conc,
                 float confidence) {
        auto toVec = [](std::initializer_list<std::string> l) -> std::vector<std::string> {
            return std::vector<std::string>(l);
        };
        auto v1 = toVec(c1), v2 = toVec(c2), vc = toVec(conc);
        
        MKRule::Condition cond1 = {v1.size()>=1?v1[0]:"", v1.size()>=2?v1[1]:"", v1.size()>=3?v1[2]:""};
        MKRule::Condition cond2 = {v2.size()>=1?v2[0]:"", v2.size()>=2?v2[1]:"", v2.size()>=3?v2[2]:""};
        MKRule::Conclusion conclusion = {vc.size()>=1?vc[0]:"", vc.size()>=2?vc[1]:"", vc.size()>=3?vc[2]:""};
        
        addRule(name, cond1, cond2, conclusion, confidence);
    }

    // ─────────────────────────────────────────
    //  CORE: RUN REASONING ON A QUERY
    // ─────────────────────────────────────────

    // Try to derive an answer using reasoning rules
    // Takes the pattern graph as input and attempts to chain rules
    std::vector<MKInference> reason(const std::string& subject, const std::string& relation,
                                     MKPatternGraph& graph) {
        std::vector<MKInference> results;
        std::string src = subject;
        std::string rel = relation;
        std::transform(src.begin(), src.end(), src.begin(), ::tolower);
        std::transform(rel.begin(), rel.end(), rel.begin(), ::tolower);

        for (const auto& rule : rules) {
            if (!rule.enabled) continue;
            if (rule.conditions.empty()) continue;
            
            // Check if this rule's conclusion matches what we're looking for
            if (rule.conclusion.relation != rel) continue;

            // Try to bind variables
            // If conclusion is "?x [rel] ?z" and we're asking about "subject [rel] ?"
            // Then ?x = subject, and we need to find ?z

            const auto& cond1 = rule.conditions[0];
            
            // Condition 1: look up subject's edges matching cond1.relation
            auto midResults = graph.query(src, cond1.relation);
            
            if (midResults.empty()) continue;

            for (const auto& mid : midResults) {
                if (rule.conditions.size() >= 2) {
                    const auto& cond2 = rule.conditions[1];
                    // Condition 2: look up the intermediate node
                    auto finalResults = graph.query(mid, cond2.relation);
                    
                    for (const auto& final : finalResults) {
                        MKInference inf;
                        inf.derived_source = src;
                        inf.derived_relation = rel;
                        inf.derived_target = final;
                        inf.confidence = rule.confidence;
                        inf.rule_name = rule.name;
                        inf.chain.push_back(src + " " + cond1.relation + " " + mid);
                        inf.chain.push_back(mid + " " + cond2.relation + " " + final);
                        inf.chain.push_back("THEREFORE: " + src + " " + rel + " " + final);
                        results.push_back(inf);
                        total_inferences++;
                    }
                } else {
                    // Single-condition rule (like symmetric inference)
                    MKInference inf;
                    inf.derived_source = mid;  // Swap for symmetric
                    inf.derived_relation = rule.conclusion.relation;
                    inf.derived_target = src;
                    inf.confidence = rule.confidence;
                    inf.rule_name = rule.name;
                    inf.chain.push_back(src + " " + cond1.relation + " " + mid);
                    inf.chain.push_back("THEREFORE: " + mid + " " + rule.conclusion.relation + " " + src);
                    results.push_back(inf);
                    total_inferences++;
                }
            }
        }

        return results;
    }

    // Try ALL rules and derive everything possible (for background enrichment)
    int deriveAll(MKPatternGraph& graph) {
        int newFacts = 0;
        // This is expensive — only run during idle time
        // For each rule, try every possible variable binding
        // (Simplified: iterate all edges and try to match rule patterns)
        
        std::cout << "[REASONING] Running full derivation pass...\n";
        // Implementation would iterate all nodes and try each rule
        // For now, this is called selectively during maintenance windows
        std::cout << "[REASONING] Derivation complete. New facts: " << newFacts << "\n";
        return newFacts;
    }

    // ─────────────────────────────────────────
    //  LOAD CUSTOM RULES FROM FILE
    // ─────────────────────────────────────────

    void loadRulesFromFile(const std::string& filename) {
        std::string path = rules_dir + "/" + filename;
        std::ifstream in(path);
        if (!in.is_open()) return;

        std::string line;
        while (std::getline(in, line)) {
            if (line.empty() || line[0] == '#') continue;
            // Format: name|cond1_sub cond1_rel cond1_obj|cond2_sub cond2_rel cond2_obj|conc_sub conc_rel conc_obj|confidence
            std::stringstream ss(line);
            std::string name, c1str, c2str, concstr, confstr;
            if (std::getline(ss, name, '|') && std::getline(ss, c1str, '|') &&
                std::getline(ss, c2str, '|') && std::getline(ss, concstr, '|') &&
                std::getline(ss, confstr, '|')) {
                
                auto cond1 = parseCondition(c1str);
                auto cond2 = parseCondition(c2str);
                MKRule::Conclusion conc;
                std::stringstream cs(concstr);
                cs >> conc.var_subject >> conc.relation >> conc.var_object;
                
                float conf = 0.8f;
                try { conf = std::stof(confstr); } catch (...) {}
                
                addRule(name, cond1, cond2, conc, conf);
            }
        }
        in.close();
        std::cout << "[REASONING] Loaded rules from: " << path << "\n";
    }

    // ─────────────────────────────────────────
    //  STATS
    // ─────────────────────────────────────────

    int ruleCount() const { return rules.size(); }
    int inferenceCount() const { return total_inferences; }

    void printStats() const {
        std::cout << "\n--- [REASONING CHAINS] ---\n";
        std::cout << "  Rules loaded: " << rules.size() << "\n";
        std::cout << "  Inferences made: " << total_inferences << "\n";
        std::cout << "--------------------------\n";
    }

    void listRules() const {
        std::cout << "\n=== ACTIVE REASONING RULES ===\n";
        for (const auto& rule : rules) {
            std::cout << "  [" << rule.id << "] " << rule.name 
                      << " (confidence=" << rule.confidence << ")\n";
        }
        std::cout << "==============================\n";
    }
};

#endif // MK_REASONING_CHAINS_CPP
