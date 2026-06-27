#ifndef MK_VERBALIZER_CPP
#define MK_VERBALIZER_CPP

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <algorithm>
#include <cmath>

// ===================================================================================
// MK NEURAL GRAPH FUSION — VERBALIZER
// ===================================================================================
// Converts graph paths and activation patterns into natural language.
// Takes activated nodes, traced paths, and edge relations, then produces
// coherent English sentences using a template library.
//
// Features:
//   - 20+ sentence templates for different relation types
//   - Confidence-based hedging (definitely/probably/possibly/might)
//   - Conjunction handling for multi-fact statements
//   - Paragraph assembly for complex multi-hop explanations
//   - Relation-aware verb selection
// ===================================================================================

// A single verbalized statement
struct MKStatement {
    std::string text;
    float confidence;
    std::string source_relation;
    int source_hops;        // How many graph hops produced this
};

// A verbalized paragraph (multiple statements combined)
struct MKParagraph {
    std::vector<MKStatement> statements;
    std::string assembled_text;
    float avg_confidence;
    int fact_count;
};

// Input: a path edge to verbalize
struct MKVerbalizerInput {
    std::string subject;
    std::string relation;
    std::string object;
    float weight;
    int hop_number;
};

class MKVerbalizer {
private:
    // Template library: relation -> list of sentence templates
    // {S} = subject, {O} = object, {H} = hedge word
    std::unordered_map<std::string, std::vector<std::string>> templates;
    
    // Hedge words based on confidence levels
    std::vector<std::pair<float, std::string>> hedge_levels;
    
    int total_verbalizations;
    int total_paragraphs;

    // Initialize the template library
    void initTemplates() {
        // Taxonomy relations
        templates["is_a"] = {
            "{S} is a type of {O}",
            "{S} is classified as {O}",
            "{S} {H} belongs to the category of {O}",
            "{S} can be described as {O}"
        };

        templates["type_of"] = {
            "{S} is a kind of {O}",
            "{S} falls under the category of {O}"
        };

        // Property relations
        templates["has"] = {
            "{S} has {O}",
            "{S} possesses {O}",
            "{S} {H} features {O}"
        };

        templates["has_property"] = {
            "{S} is characterized by {O}",
            "{S} exhibits the property of {O}",
            "A notable feature of {S} is {O}"
        };

        // Capability relations
        templates["can"] = {
            "{S} can {O}",
            "{S} is able to {O}",
            "{S} has the ability to {O}"
        };

        templates["capable_of"] = {
            "{S} is capable of {O}",
            "{S} {H} has the capacity to {O}"
        };

        // Location relations
        templates["lives_in"] = {
            "{S} lives in {O}",
            "{S} can be found in {O}",
            "{S} inhabits {O}"
        };

        templates["located_in"] = {
            "{S} is located in {O}",
            "{S} can be found in {O}",
            "You will find {S} in {O}"
        };

        // Causal relations
        templates["causes"] = {
            "{S} causes {O}",
            "{S} leads to {O}",
            "{S} {H} results in {O}"
        };

        templates["caused_by"] = {
            "{S} is caused by {O}",
            "{S} results from {O}",
            "{S} {H} occurs because of {O}"
        };

        // Part-whole relations
        templates["part_of"] = {
            "{S} is part of {O}",
            "{S} is a component of {O}",
            "{S} belongs to {O}"
        };

        templates["contains"] = {
            "{S} contains {O}",
            "{S} includes {O}",
            "{S} is composed of {O}"
        };

        // Usage relations
        templates["used_for"] = {
            "{S} is used for {O}",
            "{S} serves the purpose of {O}",
            "People use {S} to {O}"
        };

        templates["uses"] = {
            "{S} uses {O}",
            "{S} employs {O}",
            "{S} makes use of {O}"
        };

        // Temporal relations
        templates["before"] = {
            "{S} comes before {O}",
            "{S} precedes {O}",
            "{S} {H} happens prior to {O}"
        };

        templates["after"] = {
            "{S} comes after {O}",
            "{S} follows {O}",
            "{S} occurs subsequent to {O}"
        };

        // Similarity and opposition
        templates["similar_to"] = {
            "{S} is similar to {O}",
            "{S} resembles {O}",
            "{S} and {O} share common characteristics"
        };

        templates["opposite_of"] = {
            "{S} is the opposite of {O}",
            "{S} contrasts with {O}",
            "{S} and {O} are opposing concepts"
        };

        // Creation relations
        templates["created_by"] = {
            "{S} was created by {O}",
            "{S} was made by {O}",
            "{O} is the creator of {S}"
        };

        templates["creates"] = {
            "{S} creates {O}",
            "{S} produces {O}",
            "{S} is responsible for making {O}"
        };

        // Requirement relations
        templates["requires"] = {
            "{S} requires {O}",
            "{S} needs {O}",
            "{S} {H} depends on {O}"
        };

        templates["enables"] = {
            "{S} enables {O}",
            "{S} makes {O} possible",
            "Without {S}, {O} would not be achievable"
        };

        // Knowledge/learning relations
        templates["knows"] = {
            "{S} knows about {O}",
            "{S} is knowledgeable in {O}",
            "{S} has understanding of {O}"
        };

        templates["related_to"] = {
            "{S} is related to {O}",
            "{S} has a connection to {O}",
            "There is a relationship between {S} and {O}"
        };

        // Default for unknown relations
        templates["_default"] = {
            "{S} {R} {O}",
            "There is a '{R}' relationship between {S} and {O}",
            "{S} is connected to {O} through {R}"
        };
    }

    // Initialize hedge words
    void initHedges() {
        hedge_levels = {
            {0.95f, "definitely"},
            {0.85f, "very likely"},
            {0.70f, "probably"},
            {0.55f, "likely"},
            {0.40f, "possibly"},
            {0.25f, "perhaps"},
            {0.10f, "might"},
            {0.0f,  "conceivably"}
        };
    }

    // Get hedge word for confidence level
    std::string getHedge(float confidence) {
        for (const auto& level : hedge_levels) {
            if (confidence >= level.first) {
                return level.second;
            }
        }
        return "conceivably";
    }

    // Select a template based on relation (cycles through available templates)
    std::string selectTemplate(const std::string& relation, int variation = 0) {
        auto it = templates.find(relation);
        if (it == templates.end()) {
            it = templates.find("_default");
        }
        if (it == templates.end() || it->second.empty()) {
            return "{S} " + relation + " {O}";
        }
        int idx = variation % static_cast<int>(it->second.size());
        return it->second[idx];
    }

    // Fill in a template with actual values
    std::string fillTemplate(const std::string& tmpl, const std::string& subject,
                             const std::string& object, const std::string& relation,
                             float confidence) {
        std::string result = tmpl;
        std::string hedge = getHedge(confidence);
        
        // Replace placeholders
        size_t pos;
        while ((pos = result.find("{S}")) != std::string::npos) {
            result.replace(pos, 3, subject);
        }
        while ((pos = result.find("{O}")) != std::string::npos) {
            result.replace(pos, 3, object);
        }
        while ((pos = result.find("{R}")) != std::string::npos) {
            result.replace(pos, 3, relation);
        }
        while ((pos = result.find("{H}")) != std::string::npos) {
            result.replace(pos, 3, hedge);
        }
        
        // Capitalize first letter
        if (!result.empty()) {
            result[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(result[0])));
        }
        
        return result;
    }

    // Join multiple items with proper conjunction
    std::string joinWithConjunction(const std::vector<std::string>& items, 
                                     const std::string& conjunction = "and") {
        if (items.empty()) return "";
        if (items.size() == 1) return items[0];
        if (items.size() == 2) return items[0] + " " + conjunction + " " + items[1];
        
        std::string result;
        for (size_t i = 0; i < items.size(); i++) {
            if (i == items.size() - 1) {
                result += conjunction + " " + items[i];
            } else {
                result += items[i] + ", ";
            }
        }
        return result;
    }

public:
    MKVerbalizer() : total_verbalizations(0), total_paragraphs(0) {
        initTemplates();
        initHedges();
        std::cout << "[VERBALIZER] Natural language generation engine initialized.\n";
        std::cout << "  Templates: " << templates.size() << " relation types covered.\n";
    }

    // ─────────────────────────────────────────
    //  SINGLE FACT VERBALIZATION
    // ─────────────────────────────────────────

    // Verbalize a single fact/edge
    MKStatement verbalize(const std::string& subject, const std::string& relation,
                          const std::string& object, float confidence = 1.0f,
                          int variation = 0) {
        total_verbalizations++;
        MKStatement stmt;
        stmt.confidence = confidence;
        stmt.source_relation = relation;
        stmt.source_hops = 1;

        std::string tmpl = selectTemplate(relation, variation);
        stmt.text = fillTemplate(tmpl, subject, object, relation, confidence);
        
        return stmt;
    }

    // Verbalize a single edge input
    MKStatement verbalizeEdge(const MKVerbalizerInput& input) {
        MKStatement stmt = verbalize(input.subject, input.relation, input.object,
                                     input.weight, input.hop_number);
        stmt.source_hops = input.hop_number;
        return stmt;
    }

    // ─────────────────────────────────────────
    //  PATH VERBALIZATION
    // ─────────────────────────────────────────

    // Verbalize a complete path (multiple hops)
    MKParagraph verbalizePath(const std::vector<MKVerbalizerInput>& path) {
        total_paragraphs++;
        MKParagraph para;
        para.fact_count = 0;
        para.avg_confidence = 0.0f;

        if (path.empty()) {
            para.assembled_text = "No information available.";
            return para;
        }

        // Verbalize each hop
        int variation = 0;
        for (const auto& hop : path) {
            MKStatement stmt = verbalize(hop.subject, hop.relation, hop.object,
                                         hop.weight, variation++);
            stmt.source_hops = hop.hop_number;
            para.statements.push_back(stmt);
            para.avg_confidence += stmt.confidence;
            para.fact_count++;
        }

        if (para.fact_count > 0) {
            para.avg_confidence /= static_cast<float>(para.fact_count);
        }

        // Assemble into coherent paragraph
        para.assembled_text = assembleParagraph(para.statements);
        return para;
    }

    // ─────────────────────────────────────────
    //  PARAGRAPH ASSEMBLY
    // ─────────────────────────────────────────

    // Assemble multiple statements into a paragraph
    std::string assembleParagraph(const std::vector<MKStatement>& statements) {
        if (statements.empty()) return "No information available.";
        if (statements.size() == 1) return statements[0].text + ".";

        std::string paragraph;
        
        // Add confidence-based intro
        float avg_conf = 0.0f;
        for (const auto& s : statements) avg_conf += s.confidence;
        avg_conf /= static_cast<float>(statements.size());

        if (avg_conf >= 0.9f) {
            paragraph = "Based on strong evidence: ";
        } else if (avg_conf >= 0.7f) {
            paragraph = "Based on available knowledge: ";
        } else if (avg_conf >= 0.5f) {
            paragraph = "From what I can determine: ";
        } else {
            paragraph = "With limited confidence: ";
        }

        // Connect statements with transitional phrases
        std::vector<std::string> transitions = {
            "Furthermore, ", "Additionally, ", "Moreover, ",
            "Also, ", "In addition, ", "Building on that, ",
            "Following from this, ", "Consequently, ", "Related to this, "
        };

        for (size_t i = 0; i < statements.size(); i++) {
            if (i == 0) {
                paragraph += statements[i].text + ". ";
            } else {
                int t_idx = static_cast<int>((i - 1) % transitions.size());
                paragraph += transitions[t_idx] + statements[i].text + ". ";
            }
        }

        return paragraph;
    }

    // ─────────────────────────────────────────
    //  MULTI-FACT SUMMARY
    // ─────────────────────────────────────────

    // Verbalize multiple facts about the same subject
    std::string summarizeSubject(const std::string& subject,
                                  const std::vector<std::pair<std::string, std::string>>& facts,
                                  float avg_confidence = 0.8f) {
        if (facts.empty()) return "I don't have information about " + subject + ".";

        // Group facts by relation type
        std::unordered_map<std::string, std::vector<std::string>> grouped;
        for (const auto& fact : facts) {
            grouped[fact.first].push_back(fact.second);
        }

        std::string summary;
        int variation = 0;

        for (const auto& group : grouped) {
            const std::string& relation = group.first;
            const std::vector<std::string>& objects = group.second;

            if (objects.size() == 1) {
                MKStatement stmt = verbalize(subject, relation, objects[0], avg_confidence, variation++);
                summary += stmt.text + ". ";
            } else {
                // Combine multiple objects: "X has A, B, and C"
                std::string combined_obj = joinWithConjunction(objects);
                MKStatement stmt = verbalize(subject, relation, combined_obj, avg_confidence, variation++);
                summary += stmt.text + ". ";
            }
        }

        return summary;
    }

    // ─────────────────────────────────────────
    //  EXPLANATION GENERATION
    // ─────────────────────────────────────────

    // Generate a why-explanation from a path
    std::string explainPath(const std::string& source, const std::string& target,
                            const std::vector<MKVerbalizerInput>& path) {
        if (path.empty()) {
            return "I cannot explain the connection between " + source + " and " + target + ".";
        }

        std::string explanation = "The connection between " + source + " and " + target + " works like this: ";
        
        for (size_t i = 0; i < path.size(); i++) {
            const auto& hop = path[i];
            if (i == 0) {
                explanation += hop.subject + " " + hop.relation + " " + hop.object;
            } else {
                explanation += ", which " + hop.relation + " " + hop.object;
            }
        }
        explanation += ".";

        // Add confidence note
        float total_conf = 1.0f;
        for (const auto& hop : path) total_conf *= hop.weight;
        
        if (total_conf >= 0.8f) {
            explanation += " This reasoning chain has high confidence.";
        } else if (total_conf >= 0.5f) {
            explanation += " This reasoning is fairly confident but involves some uncertainty.";
        } else {
            explanation += " Note: this chain involves several uncertain steps.";
        }

        return explanation;
    }

    // ─────────────────────────────────────────
    //  STATS & CONFIG
    // ─────────────────────────────────────────

    int getTemplateCount() const {
        int count = 0;
        for (const auto& pair : templates) {
            count += static_cast<int>(pair.second.size());
        }
        return count;
    }

    int getRelationCount() const { return static_cast<int>(templates.size()); }
    int getTotalVerbalizations() const { return total_verbalizations; }
    int getTotalParagraphs() const { return total_paragraphs; }

    void printStats() const {
        std::cout << "\n--- [VERBALIZER] ---\n";
        std::cout << "  Relation types: " << templates.size() << "\n";
        int total_templates = 0;
        for (const auto& p : templates) total_templates += static_cast<int>(p.second.size());
        std::cout << "  Total templates: " << total_templates << "\n";
        std::cout << "  Verbalizations: " << total_verbalizations << "\n";
        std::cout << "  Paragraphs assembled: " << total_paragraphs << "\n";
        std::cout << "--------------------\n";
    }
};

#endif // MK_VERBALIZER_CPP
