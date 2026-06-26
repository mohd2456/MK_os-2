#ifndef MK_COMPOSER_CPP
#define MK_COMPOSER_CPP

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <ctime>
#include <cstdlib>
#include <algorithm>

// ===================================================================================
// MK HYBRID REASONING ENGINE - LAYER 4: RESPONSE COMPOSER
// ===================================================================================
// Builds natural language responses by combining templates + facts + grammar rules.
// NOT generating word-by-word like an LLM. Instead assembles sentences from blocks.
//
// Architecture:
//   1. Select response template based on intent type
//   2. Fill template slots with retrieved facts
//   3. Apply personality filter (casual/technical/friendly)
//   4. Handle "I don't know" gracefully with partial information
//
// Templates are categorized by:
//   - Intent type (question answer, fact confirmation, command response)
//   - Personality mode (casual, technical, friendly)
//   - Knowledge state (full answer, partial answer, no answer)
//
// Designed for extremely low-end hardware. Pure C++ STL.
// ===================================================================================

// Personality modes for response style
enum class MKPersonality {
    CASUAL,         // Relaxed, short responses
    TECHNICAL,      // Precise, detailed responses
    FRIENDLY        // Warm, encouraging responses
};

// What we know when composing a response
struct MKResponseContext {
    std::string subject;
    std::string relation;
    std::string object;
    std::vector<std::string> facts_found;       // Direct facts retrieved
    std::vector<std::string> inferred_facts;    // Facts from reasoning
    std::string reasoning_chain;                // How we got the answer
    bool is_partial;                            // Did we find SOME but not all info?
    float confidence;
};

class MKComposer {
private:
    MKPersonality personality;
    int total_composed;
    std::string mk_name;

    // Template collections organized by category
    std::vector<std::string> answer_templates;
    std::vector<std::string> confirm_templates;
    std::vector<std::string> unknown_templates;
    std::vector<std::string> partial_templates;
    std::vector<std::string> greeting_templates;
    std::vector<std::string> help_templates;
    std::vector<std::string> error_templates;
    std::vector<std::string> fact_list_templates;

    // Simple pseudo-random selection (deterministic enough for our purposes)
    int pickRandom(int max) {
        static unsigned int seed = static_cast<unsigned int>(std::time(nullptr));
        seed = seed * 1103515245 + 12345;
        return (seed / 65536) % max;
    }

    // Pick a random template from a collection
    std::string pickTemplate(const std::vector<std::string>& templates) {
        if (templates.empty()) return "";
        return templates[pickRandom(templates.size())];
    }

    // Replace {placeholder} in template with actual values
    std::string fillTemplate(const std::string& tmpl, 
                            const std::unordered_map<std::string, std::string>& vars) {
        std::string result = tmpl;
        for (const auto& pair : vars) {
            std::string placeholder = "{" + pair.first + "}";
            size_t pos = 0;
            while ((pos = result.find(placeholder, pos)) != std::string::npos) {
                result.replace(pos, placeholder.length(), pair.second);
                pos += pair.second.length();
            }
        }
        return result;
    }

    // Capitalize first letter
    std::string capitalize(const std::string& s) {
        if (s.empty()) return s;
        std::string result = s;
        result[0] = std::toupper(result[0]);
        return result;
    }

    // Join a list with commas and "and"
    std::string joinList(const std::vector<std::string>& items) {
        if (items.empty()) return "";
        if (items.size() == 1) return items[0];
        if (items.size() == 2) return items[0] + " and " + items[1];

        std::string result;
        for (size_t i = 0; i < items.size(); i++) {
            if (i > 0) {
                if (i == items.size() - 1) {
                    result += ", and ";
                } else {
                    result += ", ";
                }
            }
            result += items[i];
        }
        return result;
    }

public:
    MKComposer(MKPersonality mode = MKPersonality::FRIENDLY) 
        : personality(mode), total_composed(0), mk_name("MK") {
        loadTemplates();
        std::cout << "[COMPOSER] Response composer initialized (mode: " 
                  << personalityName() << ").\n";
    }

    // ─────────────────────────────────────────
    //  TEMPLATE INITIALIZATION
    // ─────────────────────────────────────────

    void loadTemplates() {
        // Answer templates - when we have a direct answer
        answer_templates = {
            "{subject} {relation} {object}.",
            "Based on what I know, {subject} {relation} {object}.",
            "I believe {subject} {relation} {object}.",
            "From my knowledge: {subject} {relation} {object}.",
            "Yes! {subject} {relation} {object}.",
            "That would be {object}. {subject} {relation} {object}."
        };

        // Confirmation templates - when learning a new fact
        confirm_templates = {
            "Got it! I'll remember that {subject} {relation} {object}.",
            "Noted. {subject} {relation} {object}. Stored in my knowledge graph.",
            "Learned! {subject} {relation} {object}.",
            "I've added that to my memory: {subject} {relation} {object}.",
            "Cool, I now know that {subject} {relation} {object}.",
            "Remembered! I won't forget that {subject} {relation} {object}."
        };

        // Unknown templates - when we have no information
        unknown_templates = {
            "I don't have any information about {subject} yet. You can teach me!",
            "Hmm, I don't know about {subject}. Tell me and I'll remember.",
            "That's not in my knowledge graph yet. What should I know about {subject}?",
            "I haven't learned about {subject}. Want to teach me?",
            "No data on {subject} in my memory. I'm always ready to learn though!",
            "I'm drawing a blank on {subject}. Help me learn?"
        };

        // Partial templates - when we found SOME info but not exactly what was asked
        partial_templates = {
            "I'm not sure about that specifically, but here's what I know about {subject}: {facts}",
            "I don't have a direct answer, but I do know: {facts}",
            "Partially! Here's what I've got on {subject}: {facts}",
            "I know some things about {subject}: {facts}. Want to tell me more?",
            "Not exactly what you asked, but related: {facts}"
        };

        // Greeting templates
        greeting_templates = {
            "Hey there! I'm MK. Ask me anything or teach me something new.",
            "Hello! MK here, ready to think. What's on your mind?",
            "Hi! I'm MK, your knowledge companion. What can I help with?",
            "Yo! MK online and thinking. What do you want to know?",
            "Hey! Ready to chat, learn, or answer questions."
        };

        // Help templates
        help_templates = {
            "I'm MK, a hybrid reasoning engine. Here's what I can do:\n"
            "  - Answer questions about what I know (ask me anything!)\n"
            "  - Learn new facts (just tell me: 'X is Y' or 'remember X has Y')\n"
            "  - Reason through chains (I can infer new facts from existing ones)\n"
            "  - Commands: help, status, stats, rules, dump, quit\n"
            "Try asking: 'what is a cat?' or tell me: 'the sky is blue'"
        };

        // Error templates
        error_templates = {
            "Something went wrong on my end. Could you rephrase that?",
            "I got confused parsing that. Try saying it differently?",
            "Hmm, I couldn't quite understand. Can you rephrase?"
        };

        // Fact list templates - for listing multiple facts
        fact_list_templates = {
            "Here's what I know about {subject}:\n{fact_list}",
            "Everything I know about {subject}:\n{fact_list}",
            "My knowledge on {subject}:\n{fact_list}"
        };
    }

    // ─────────────────────────────────────────
    //  CORE: COMPOSE RESPONSES
    // ─────────────────────────────────────────

    // Compose an answer to a question
    std::string composeAnswer(const MKResponseContext& ctx) {
        total_composed++;
        std::unordered_map<std::string, std::string> vars;
        vars["subject"] = ctx.subject;
        vars["relation"] = humanizeRelation(ctx.relation);
        vars["object"] = ctx.object;

        // Case 1: Direct answer found
        if (!ctx.facts_found.empty()) {
            if (ctx.facts_found.size() == 1) {
                vars["object"] = ctx.facts_found[0];
                return fillTemplate(pickTemplate(answer_templates), vars);
            } else {
                // Multiple answers
                vars["object"] = joinList(ctx.facts_found);
                return fillTemplate(pickTemplate(answer_templates), vars);
            }
        }

        // Case 2: Inferred answer
        if (!ctx.inferred_facts.empty()) {
            vars["object"] = ctx.inferred_facts[0];
            std::string answer = fillTemplate(pickTemplate(answer_templates), vars);
            if (!ctx.reasoning_chain.empty()) {
                answer += " (Reasoned via: " + ctx.reasoning_chain + ")";
            }
            return answer;
        }

        // Case 3: Partial information
        if (ctx.is_partial && !ctx.facts_found.empty()) {
            vars["facts"] = joinList(ctx.facts_found);
            return fillTemplate(pickTemplate(partial_templates), vars);
        }

        // Case 4: No information at all
        return fillTemplate(pickTemplate(unknown_templates), vars);
    }

    // Compose a confirmation for learning a fact
    std::string composeConfirmation(const std::string& subject, const std::string& relation,
                                    const std::string& object) {
        total_composed++;
        std::unordered_map<std::string, std::string> vars;
        vars["subject"] = subject;
        vars["relation"] = humanizeRelation(relation);
        vars["object"] = object;
        return fillTemplate(pickTemplate(confirm_templates), vars);
    }

    // Compose a greeting response
    std::string composeGreeting() {
        total_composed++;
        return pickTemplate(greeting_templates);
    }

    // Compose help text
    std::string composeHelp() {
        total_composed++;
        return pickTemplate(help_templates);
    }

    // Compose an error/fallback response
    std::string composeError() {
        total_composed++;
        return pickTemplate(error_templates);
    }

    // Compose a fact list (for "tell me about X" or "show X")
    std::string composeFactList(const std::string& subject,
                                const std::vector<std::pair<std::string, std::string>>& facts) {
        total_composed++;

        if (facts.empty()) {
            std::unordered_map<std::string, std::string> vars;
            vars["subject"] = subject;
            return fillTemplate(pickTemplate(unknown_templates), vars);
        }

        std::string factList;
        for (const auto& fact : facts) {
            factList += "  - " + capitalize(subject) + " " + humanizeRelation(fact.first) 
                     + " " + fact.second + "\n";
        }

        std::unordered_map<std::string, std::string> vars;
        vars["subject"] = subject;
        vars["fact_list"] = factList;
        return fillTemplate(pickTemplate(fact_list_templates), vars);
    }

    // Compose a multi-fact paragraph response
    std::string composeParagraph(const std::string& subject,
                                 const std::vector<std::pair<std::string, std::string>>& facts) {
        total_composed++;
        if (facts.empty()) return "I don't know anything about " + subject + " yet.";

        std::string paragraph = capitalize(subject);
        for (size_t i = 0; i < facts.size(); i++) {
            if (i == 0) {
                paragraph += " " + humanizeRelation(facts[i].first) + " " + facts[i].second;
            } else if (i == facts.size() - 1) {
                paragraph += ", and " + humanizeRelation(facts[i].first) + " " + facts[i].second;
            } else {
                paragraph += ", " + humanizeRelation(facts[i].first) + " " + facts[i].second;
            }
        }
        paragraph += ".";
        return paragraph;
    }

    // Compose status report
    std::string composeStatus(int nodes, int edges, int rules, int queries) {
        total_composed++;
        std::string status = "=== MK Status Report ===\n";
        status += "  Knowledge nodes: " + std::to_string(nodes) + "\n";
        status += "  Facts (edges): " + std::to_string(edges) + "\n";
        status += "  Reasoning rules: " + std::to_string(rules) + "\n";
        status += "  Queries answered: " + std::to_string(queries) + "\n";
        status += "  Responses composed: " + std::to_string(total_composed) + "\n";
        status += "========================";
        return status;
    }

    // ─────────────────────────────────────────
    //  PERSONALITY & HUMANIZATION
    // ─────────────────────────────────────────

    // Convert graph relation names to natural English
    std::string humanizeRelation(const std::string& relation) {
        static std::unordered_map<std::string, std::string> humanMap = {
            {"is_a", "is"},
            {"has", "has"},
            {"can", "can"},
            {"lives_in", "lives in"},
            {"located_in", "is located in"},
            {"part_of", "is part of"},
            {"created", "created"},
            {"created_by", "was created by"},
            {"likes", "likes"},
            {"dislikes", "dislikes"},
            {"owns", "owns"},
            {"works_at", "works at"},
            {"needs", "needs"},
            {"wants", "wants"},
            {"knows", "knows"},
            {"uses", "uses"},
            {"type_of", "is a type of"},
            {"kind_of", "is a kind of"},
            {"has_property", "has the property of being"},
            {"opposite_of", "is the opposite of"},
            {"color", "is colored"},
            {"made_of", "is made of"},
            {"used_for", "is used for"},
            {"eats", "eats"},
            {"produces", "produces"},
            {"contains", "contains"},
            {"larger_than", "is larger than"},
            {"smaller_than", "is smaller than"},
            {"faster_than", "is faster than"},
            {"related_to", "is related to"},
            {"example_of", "is an example of"},
            {"built_with", "was built with"},
            {"runs_on", "runs on"}
        };

        auto it = humanMap.find(relation);
        if (it != humanMap.end()) return it->second;
        
        // Fallback: replace underscores with spaces
        std::string result = relation;
        std::replace(result.begin(), result.end(), '_', ' ');
        return result;
    }

    // Set personality mode
    void setPersonality(MKPersonality mode) {
        personality = mode;
        std::cout << "[COMPOSER] Personality set to: " << personalityName() << "\n";
    }

    std::string personalityName() const {
        switch (personality) {
            case MKPersonality::CASUAL: return "casual";
            case MKPersonality::TECHNICAL: return "technical";
            case MKPersonality::FRIENDLY: return "friendly";
            default: return "unknown";
        }
    }

    // ─────────────────────────────────────────
    //  STATS
    // ─────────────────────────────────────────

    int composeCount() const { return total_composed; }

    void printStats() const {
        std::cout << "\n--- [COMPOSER] ---\n";
        std::cout << "  Responses composed: " << total_composed << "\n";
        std::cout << "  Personality: " << personalityName() << "\n";
        std::cout << "------------------\n";
    }
};

#endif // MK_COMPOSER_CPP
