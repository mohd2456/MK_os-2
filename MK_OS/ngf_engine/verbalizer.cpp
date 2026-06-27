#ifndef MK_VERBALIZER_CPP
#define MK_VERBALIZER_CPP

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <chrono>

// ===================================================================================
// MK NEURAL GRAPH FUSION - VERBALIZER
// ===================================================================================
// Converts a graph path (reasoning chain) into natural English sentences. Maps
// relationship types to natural language phrases and combines multiple path segments
// into coherent paragraphs.
//
// Relation mappings:
//   causes -> "because" / "which causes"
//   enables -> "which allows" / "enabling"
//   part_of -> "which is part of"
//   is_a -> "is a type of" / "is a kind of"
//   has_property -> "which has the property of"
//   opposite_of -> "which is the opposite of"
//   used_for -> "which is used for"
//   made_of -> "which is made of"
//   located_in -> "which is found in"
//   etc.
//
// Features:
//   - Multiple sentence templates per relation
//   - Confidence-based hedging (definitely, probably, possibly)
//   - Paragraph assembly from multiple paths
//   - Natural connectors (furthermore, additionally, also)
//   - Pronoun resolution for repeated concepts
// ===================================================================================

// A verbalized segment (one relationship expressed in English)
struct MKVerbalSegment {
    std::string text;
    float confidence;
    std::string source_concept;
    std::string target_concept;
    std::string relation;
};

// Complete verbalized output
struct MKVerbalOutput {
    std::string full_text;              // Complete paragraph
    std::string short_answer;           // One-sentence summary
    std::vector<MKVerbalSegment> segments;
    float overall_confidence;
    int facts_used;
    std::string reasoning_summary;      // Brief logic chain

    MKVerbalOutput() : overall_confidence(0.0f), facts_used(0) {}
};

// The main verbalizer class
class MKVerbalizer {
private:
    // Relation -> list of templates. {source} and {target} are placeholders.
    std::unordered_map<std::string, std::vector<std::string>> templates_;

    // Confidence hedging phrases
    std::vector<std::pair<float, std::string>> confidence_hedges_;

    // Sentence connectors for multi-fact paragraphs
    std::vector<std::string> connectors_;

    // Conclusion starters
    std::vector<std::string> conclusion_starters_;

    // Initialize all templates
    void initTemplates() {
        templates_["is_a"] = {
            "{source} is a type of {target}",
            "{source} is a kind of {target}",
            "{source} is classified as {target}",
            "{source} belongs to the category of {target}",
            "{source} falls under {target}"
        };
        templates_["causes"] = {
            "{source} causes {target}",
            "{source} leads to {target}",
            "{source} results in {target}",
            "{source} brings about {target}",
            "{source} triggers {target}"
        };
        templates_["enables"] = {
            "{source} enables {target}",
            "{source} allows {target}",
            "{source} makes {target} possible",
            "{source} facilitates {target}",
            "{source} permits {target}"
        };
        templates_["prevents"] = {
            "{source} prevents {target}",
            "{source} blocks {target}",
            "{source} stops {target}",
            "{source} inhibits {target}",
            "{source} hinders {target}"
        };
        templates_["part_of"] = {
            "{source} is part of {target}",
            "{source} is a component of {target}",
            "{source} belongs to {target}",
            "{source} is included in {target}",
            "{source} forms part of {target}"
        };
        templates_["has_property"] = {
            "{source} has the property of being {target}",
            "{source} is characterized by {target}",
            "{source} exhibits {target}",
            "{source} possesses {target}",
            "{source} is known for {target}"
        };
        templates_["opposite_of"] = {
            "{source} is the opposite of {target}",
            "{source} is contrary to {target}",
            "{source} contrasts with {target}",
            "{source} is the antithesis of {target}",
            "{source} opposes {target}"
        };
        templates_["used_for"] = {
            "{source} is used for {target}",
            "{source} serves the purpose of {target}",
            "{source} is employed in {target}",
            "{source} is utilized for {target}",
            "{source} functions as {target}"
        };
        templates_["made_of"] = {
            "{source} is made of {target}",
            "{source} is composed of {target}",
            "{source} consists of {target}",
            "{source} is constructed from {target}",
            "{source} contains {target}"
        };
        templates_["located_in"] = {
            "{source} is located in {target}",
            "{source} is found in {target}",
            "{source} exists in {target}",
            "{source} resides in {target}",
            "{source} can be found in {target}"
        };
        templates_["requires"] = {
            "{source} requires {target}",
            "{source} needs {target}",
            "{source} depends on {target}",
            "{source} cannot function without {target}",
            "{source} is dependent upon {target}"
        };
        templates_["produces"] = {
            "{source} produces {target}",
            "{source} generates {target}",
            "{source} creates {target}",
            "{source} yields {target}",
            "{source} outputs {target}"
        };
        templates_["similar_to"] = {
            "{source} is similar to {target}",
            "{source} resembles {target}",
            "{source} is comparable to {target}",
            "{source} is like {target}",
            "{source} shares characteristics with {target}"
        };
        templates_["stronger_than"] = {
            "{source} is stronger than {target}",
            "{source} surpasses {target}",
            "{source} exceeds {target} in strength",
            "{source} is more powerful than {target}"
        };
        templates_["weaker_than"] = {
            "{source} is weaker than {target}",
            "{source} is less potent than {target}",
            "{source} falls short of {target}",
            "{source} is less effective than {target}"
        };
        templates_["contains"] = {
            "{source} contains {target}",
            "{source} includes {target}",
            "{source} encompasses {target}",
            "{source} holds {target}",
            "{source} incorporates {target}"
        };
        templates_["precedes"] = {
            "{source} comes before {target}",
            "{source} precedes {target}",
            "{source} happens before {target}",
            "{source} leads up to {target}",
            "{source} is followed by {target}"
        };
        templates_["follows"] = {
            "{source} comes after {target}",
            "{source} follows {target}",
            "{source} happens after {target}",
            "{source} succeeds {target}",
            "{source} is preceded by {target}"
        };
        templates_["implies"] = {
            "{source} implies {target}",
            "{source} suggests {target}",
            "{source} indicates {target}",
            "if {source} then {target}",
            "{source} means {target}"
        };
        templates_["contradicts"] = {
            "{source} contradicts {target}",
            "{source} conflicts with {target}",
            "{source} is inconsistent with {target}",
            "{source} opposes {target}",
            "{source} is at odds with {target}"
        };
        templates_["supports"] = {
            "{source} supports {target}",
            "{source} backs up {target}",
            "{source} corroborates {target}",
            "{source} reinforces {target}",
            "{source} provides evidence for {target}"
        };
        templates_["derived_from"] = {
            "{source} is derived from {target}",
            "{source} originates from {target}",
            "{source} comes from {target}",
            "{source} has its roots in {target}",
            "{source} evolved from {target}"
        };
        templates_["example_of"] = {
            "{source} is an example of {target}",
            "{source} exemplifies {target}",
            "{source} illustrates {target}",
            "{source} demonstrates {target}",
            "{source} is a case of {target}"
        };
        templates_["synonym_of"] = {
            "{source} is a synonym of {target}",
            "{source} means the same as {target}",
            "{source} is equivalent to {target}",
            "{source} is another word for {target}"
        };
        // Generic fallback
        templates_["unknown"] = {
            "{source} is related to {target}",
            "{source} connects to {target}",
            "{source} is associated with {target}",
            "there is a relationship between {source} and {target}"
        };
    }

    void initHedges() {
        confidence_hedges_ = {
            {0.9f, ""},                    // Very confident: no hedge needed
            {0.75f, "It appears that "},
            {0.6f, "It is likely that "},
            {0.45f, "It seems that "},
            {0.3f, "It is possible that "},
            {0.15f, "It might be that "},
            {0.0f, "Uncertain, but perhaps "}
        };
    }

    void initConnectors() {
        connectors_ = {
            "Furthermore, ", "Additionally, ", "Moreover, ",
            "Also, ", "In addition, ", "Building on this, ",
            "Related to this, ", "Similarly, ", "Along these lines, ",
            "Following from this, ", "This connects to the fact that ",
            "It is also worth noting that "
        };
        conclusion_starters_ = {
            "Therefore, ", "In conclusion, ", "To summarize, ",
            "Overall, ", "Putting this together, ", "Based on this reasoning, ",
            "So, ", "Thus, ", "Consequently, ", "As a result, "
        };
    }

    // Replace {source} and {target} in a template
    std::string fillTemplate(const std::string& tmpl, const std::string& source,
                            const std::string& target) const {
        std::string result = tmpl;
        size_t pos;
        while ((pos = result.find("{source}")) != std::string::npos) {
            result.replace(pos, 8, formatConcept(source));
        }
        while ((pos = result.find("{target}")) != std::string::npos) {
            result.replace(pos, 8, formatConcept(target));
        }
        return result;
    }

    // Format a concept name for display (replace underscores with spaces)
    std::string formatConcept(const std::string& concept) const {
        std::string result = concept;
        std::replace(result.begin(), result.end(), '_', ' ');
        return result;
    }

    // Capitalize first letter of a sentence
    std::string capitalize(const std::string& s) const {
        if (s.empty()) return s;
        std::string result = s;
        result[0] = std::toupper(result[0]);
        return result;
    }

    // Get hedge phrase based on confidence
    std::string getHedge(float confidence) const {
        for (const auto& [threshold, phrase] : confidence_hedges_) {
            if (confidence >= threshold) return phrase;
        }
        return "Perhaps ";
    }

    // Select a template (rotate through available ones for variety)
    std::string selectTemplate(const std::string& relation, int index) const {
        auto it = templates_.find(relation);
        if (it == templates_.end()) {
            it = templates_.find("unknown");
        }
        if (it == templates_.end() || it->second.empty()) {
            return "{source} is related to {target}";
        }
        return it->second[index % it->second.size()];
    }

    // Get a connector for multi-sentence paragraphs
    std::string getConnector(int sentence_index) const {
        if (sentence_index == 0) return "";
        return connectors_[(sentence_index - 1) % connectors_.size()];
    }

public:
    MKVerbalizer() {
        std::cout << "[NGF Verbalizer] Initialized verbalizer with templates" << std::endl;
        initTemplates();
        initHedges();
        initConnectors();
    }

    // Verbalize a single relationship
    std::string verbalizeSingle(const std::string& source, const std::string& target,
                               const std::string& relation, float confidence = 0.8f,
                               int template_index = 0) {
        std::string tmpl = selectTemplate(relation, template_index);
        std::string sentence = fillTemplate(tmpl, source, target);
        std::string hedge = getHedge(confidence);
        if (!hedge.empty()) {
            sentence = hedge + sentence;
        }
        return capitalize(sentence) + ".";
    }

    // Verbalize a complete reasoning path
    MKVerbalOutput verbalizePath(const std::vector<std::string>& concepts,
                                const std::vector<std::string>& relations,
                                const std::vector<float>& confidences) {
        MKVerbalOutput output;
        if (concepts.empty()) return output;

        std::ostringstream paragraph;
        int facts_count = 0;
        float conf_sum = 0.0f;

        for (size_t i = 0; i < concepts.size() - 1 && i < relations.size(); i++) {
            std::string connector = getConnector(facts_count);
            float conf = (i < confidences.size()) ? confidences[i] : 0.7f;

            std::string sentence = verbalizeSingle(
                concepts[i], concepts[i + 1], relations[i], conf, (int)i);

            MKVerbalSegment seg;
            seg.text = sentence;
            seg.confidence = conf;
            seg.source_concept = concepts[i];
            seg.target_concept = concepts[i + 1];
            seg.relation = relations[i];
            output.segments.push_back(seg);

            if (!connector.empty()) {
                paragraph << connector;
                // Lowercase the sentence start after a connector
                if (!sentence.empty()) {
                    sentence[0] = std::tolower(sentence[0]);
                }
            }
            paragraph << sentence << " ";

            conf_sum += conf;
            facts_count++;
        }

        output.full_text = paragraph.str();
        output.facts_used = facts_count;
        output.overall_confidence = facts_count > 0 ? conf_sum / facts_count : 0.0f;

        // Generate short answer (first and last concepts)
        if (concepts.size() >= 2) {
            std::string first = formatConcept(concepts.front());
            std::string last = formatConcept(concepts.back());
            std::ostringstream short_ans;
            short_ans << capitalize(first) << " connects to " << last
                      << " through " << facts_count << " reasoning step"
                      << (facts_count > 1 ? "s" : "") << ".";
            output.short_answer = short_ans.str();
        }

        // Generate reasoning summary
        std::ostringstream summary;
        summary << "Path: ";
        for (size_t i = 0; i < concepts.size(); i++) {
            summary << formatConcept(concepts[i]);
            if (i < concepts.size() - 1 && i < relations.size()) {
                summary << " -> [" << relations[i] << "] -> ";
            }
        }
        output.reasoning_summary = summary.str();

        return output;
    }

    // Verbalize multiple paths into a coherent paragraph
    MKVerbalOutput verbalizeMultiplePaths(
        const std::vector<std::vector<std::string>>& all_concepts,
        const std::vector<std::vector<std::string>>& all_relations,
        const std::vector<std::vector<float>>& all_confidences) {

        MKVerbalOutput output;
        if (all_concepts.empty()) return output;

        std::ostringstream full_text;
        int total_facts = 0;
        float total_conf = 0.0f;

        for (size_t p = 0; p < all_concepts.size(); p++) {
            MKVerbalOutput path_output = verbalizePath(
                all_concepts[p],
                p < all_relations.size() ? all_relations[p] : std::vector<std::string>{},
                p < all_confidences.size() ? all_confidences[p] : std::vector<float>{});

            if (p > 0 && !path_output.full_text.empty()) {
                full_text << "\n";
                if (p == all_concepts.size() - 1) {
                    full_text << conclusion_starters_[p % conclusion_starters_.size()];
                } else {
                    full_text << connectors_[p % connectors_.size()];
                }
            }
            full_text << path_output.full_text;

            for (const auto& seg : path_output.segments) {
                output.segments.push_back(seg);
            }
            total_facts += path_output.facts_used;
            total_conf += path_output.overall_confidence * path_output.facts_used;
        }

        output.full_text = full_text.str();
        output.facts_used = total_facts;
        output.overall_confidence = total_facts > 0 ? total_conf / total_facts : 0.0f;

        // Short answer from first path
        if (!all_concepts.empty() && all_concepts[0].size() >= 2) {
            output.short_answer = formatConcept(all_concepts[0].front()) +
                " relates to " + formatConcept(all_concepts[0].back()) + ".";
        }

        return output;
    }

    // Generate a conclusion sentence
    std::string generateConclusion(const std::string& topic, float confidence) {
        std::ostringstream oss;
        std::string starter = conclusion_starters_[
            std::hash<std::string>{}(topic) % conclusion_starters_.size()];
        oss << starter;

        if (confidence > 0.85f) {
            oss << "we can confidently say that " << formatConcept(topic)
                << " is well-supported by the available evidence.";
        } else if (confidence > 0.6f) {
            oss << "it appears that " << formatConcept(topic)
                << " is supported by multiple lines of reasoning.";
        } else if (confidence > 0.4f) {
            oss << "there is some evidence regarding " << formatConcept(topic)
                << ", though the picture is not entirely clear.";
        } else {
            oss << "the evidence regarding " << formatConcept(topic)
                << " is limited, and more information would help.";
        }
        return oss.str();
    }

    // Format a direct answer to a question
    std::string formatDirectAnswer(const std::string& question_concept,
                                   const std::string& answer_concept,
                                   const std::string& relation,
                                   float confidence) {
        std::string hedge = getHedge(confidence);
        std::string sentence = verbalizeSingle(question_concept, answer_concept,
                                              relation, confidence, 0);
        return sentence;
    }

    // Get available template count (for stats)
    size_t getTemplateCount() const {
        size_t count = 0;
        for (const auto& [rel, tmpls] : templates_) {
            count += tmpls.size();
        }
        return count;
    }

    // Get relation count
    size_t getRelationCount() const {
        return templates_.size();
    }

    // Print stats
    void printStats() const {
        std::cout << "[NGF Verbalizer] === Stats ===" << std::endl;
        std::cout << "  Relation types: " << templates_.size() << std::endl;
        size_t total = 0;
        for (const auto& [rel, tmpls] : templates_) {
            total += tmpls.size();
        }
        std::cout << "  Total templates: " << total << std::endl;
        std::cout << "  Connectors: " << connectors_.size() << std::endl;
        std::cout << "  Conclusion starters: " << conclusion_starters_.size() << std::endl;
    }
};

#endif // MK_VERBALIZER_CPP
