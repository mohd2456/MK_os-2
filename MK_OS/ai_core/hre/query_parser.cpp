#ifndef MK_QUERY_PARSER_CPP
#define MK_QUERY_PARSER_CPP

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <unordered_set>
#include <unordered_map>

// ===================================================================================
// MK HYBRID REASONING ENGINE - LAYER 3: QUERY PARSER
// ===================================================================================
// Understands user input by extracting intent, subject, action, and context.
// Uses keyword matching and pattern detection - NOT neural networks.
//
// Input flow:
//   Raw text -> Tokenize -> Detect intent type -> Extract subject/object -> Return parsed query
//
// Intent types:
//   QUESTION  - "what is X?", "who created Y?", "does X have Y?"
//   STATEMENT - "X is Y", "X has Y", "I like X" (fact to learn)
//   COMMAND   - "remember X", "forget X", "help", "status"
//   UNKNOWN   - Could not determine intent
//
// Designed for extremely low-end hardware. No regex libraries, no NLP models.
// Pure string matching with smart heuristics.
// ===================================================================================

// The type of user intent detected
enum class MKIntentType {
    QUESTION,       // User is asking something
    STATEMENT,      // User is declaring a fact
    COMMAND,        // User is giving an instruction
    GREETING,       // User is saying hello/bye
    UNKNOWN         // Could not determine
};

// Parsed representation of user input
struct MKParsedQuery {
    MKIntentType intent;
    std::string intent_name;        // Human-readable intent type
    std::string raw_input;          // Original input text
    std::string subject;            // Main subject extracted
    std::string relation;           // Relationship/action detected
    std::string object;             // Object/target of the relation
    std::string question_word;      // If question: what/who/where/when/why/how
    std::string command_verb;       // If command: remember/forget/help/etc.
    std::vector<std::string> keywords;  // All significant words extracted
    float confidence;               // How confident we are in this parse
};

class MKQueryParser {
private:
    // Word sets for classification
    std::unordered_set<std::string> question_words;
    std::unordered_set<std::string> command_words;
    std::unordered_set<std::string> greeting_words;
    std::unordered_set<std::string> stop_words;
    std::unordered_set<std::string> linking_verbs;
    std::unordered_set<std::string> possession_verbs;

    // Relation mapping: natural language verb -> graph relation
    std::unordered_map<std::string, std::string> verb_to_relation;

    int total_parsed;

    // ─────────────────────────────────────────
    //  UTILITY FUNCTIONS
    // ─────────────────────────────────────────

    // Convert string to lowercase
    std::string toLower(const std::string& s) {
        std::string result = s;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

    // Trim whitespace from both ends
    std::string trim(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    }

    // Remove punctuation from end of string
    std::string stripPunctuation(const std::string& s) {
        std::string result = s;
        while (!result.empty() && (result.back() == '?' || result.back() == '!' ||
               result.back() == '.' || result.back() == ',')) {
            result.pop_back();
        }
        return result;
    }

    // Split string into words
    std::vector<std::string> tokenize(const std::string& text) {
        std::vector<std::string> tokens;
        std::stringstream ss(text);
        std::string word;
        while (ss >> word) {
            word = stripPunctuation(toLower(word));
            if (!word.empty()) {
                tokens.push_back(word);
            }
        }
        return tokens;
    }

    // Extract keywords (non-stop words)
    std::vector<std::string> extractKeywords(const std::vector<std::string>& tokens) {
        std::vector<std::string> keywords;
        for (const auto& token : tokens) {
            if (stop_words.find(token) == stop_words.end() &&
                question_words.find(token) == question_words.end() &&
                command_words.find(token) == command_words.end()) {
                keywords.push_back(token);
            }
        }
        return keywords;
    }

    // Check if input ends with question mark
    bool endsWithQuestion(const std::string& input) {
        std::string trimmed = trim(input);
        return !trimmed.empty() && trimmed.back() == '?';
    }

    // Check if first word is a question word
    bool startsWithQuestionWord(const std::vector<std::string>& tokens) {
        if (tokens.empty()) return false;
        return question_words.find(tokens[0]) != question_words.end();
    }

    // Map a verb/word to a graph relation
    std::string mapToRelation(const std::string& verb) {
        auto it = verb_to_relation.find(verb);
        if (it != verb_to_relation.end()) return it->second;
        return verb;  // Use the word itself as the relation
    }

public:
    MKQueryParser() : total_parsed(0) {
        // Initialize question words
        question_words = {"what", "who", "where", "when", "why", "how",
                         "does", "is", "are", "can", "could", "will",
                         "would", "do", "did", "has", "have", "which"};

        // Initialize command words
        command_words = {"remember", "forget", "learn", "tell", "show",
                        "list", "help", "status", "quit", "exit", "bye",
                        "dump", "save", "load", "stats", "rules", "reset",
                        "teach", "explain"};

        // Initialize greeting words
        greeting_words = {"hello", "hi", "hey", "howdy", "yo", "sup",
                         "greetings", "goodbye", "bye", "later", "thanks",
                         "thank", "morning", "evening", "night"};

        // Initialize stop words (common words to skip when extracting keywords)
        stop_words = {"a", "an", "the", "is", "are", "was", "were", "be",
                     "been", "being", "have", "has", "had", "do", "does",
                     "did", "will", "would", "could", "should", "may",
                     "might", "shall", "can", "need", "dare", "ought",
                     "used", "to", "of", "in", "for", "on", "with", "at",
                     "by", "from", "as", "into", "through", "during",
                     "before", "after", "above", "below", "between",
                     "and", "but", "or", "nor", "not", "so", "yet",
                     "both", "either", "neither", "each", "every",
                     "all", "any", "few", "more", "most", "other",
                     "some", "such", "no", "only", "own", "same",
                     "than", "too", "very", "just", "about", "also",
                     "it", "its", "that", "this", "these", "those",
                     "i", "me", "my", "myself", "we", "our", "you",
                     "your", "he", "him", "his", "she", "her", "they",
                     "them", "their"};

        // Linking verbs (for "X is Y" pattern detection)
        linking_verbs = {"is", "are", "was", "were", "am", "equals", "means"};

        // Possession verbs (for "X has Y" pattern detection)
        possession_verbs = {"has", "have", "had", "owns", "contains", "includes"};

        // Map natural language to graph relations
        verb_to_relation = {
            {"is", "is_a"}, {"are", "is_a"}, {"was", "is_a"},
            {"has", "has"}, {"have", "has"}, {"had", "has"},
            {"can", "can"}, {"could", "can"},
            {"likes", "likes"}, {"like", "likes"}, {"loves", "likes"},
            {"lives", "lives_in"}, {"live", "lives_in"},
            {"works", "works_at"}, {"work", "works_at"},
            {"created", "created"}, {"made", "created"}, {"built", "created"},
            {"owns", "owns"}, {"own", "owns"},
            {"needs", "needs"}, {"need", "needs"},
            {"wants", "wants"}, {"want", "wants"},
            {"knows", "knows"}, {"know", "knows"},
            {"uses", "uses"}, {"use", "uses"},
            {"hates", "dislikes"}, {"hate", "dislikes"}, {"dislikes", "dislikes"},
            {"located", "located_in"}, {"found", "located_in"},
            {"part", "part_of"}, {"belongs", "part_of"},
            {"opposite", "opposite_of"}, {"unlike", "opposite_of"}
        };

        std::cout << "[QUERY PARSER] Natural language parser initialized.\n";
    }

    // ─────────────────────────────────────────
    //  CORE: PARSE USER INPUT
    // ─────────────────────────────────────────

    MKParsedQuery parse(const std::string& input) {
        total_parsed++;
        MKParsedQuery result;
        result.raw_input = input;
        result.confidence = 0.0f;

        std::string cleaned = trim(input);
        if (cleaned.empty()) {
            result.intent = MKIntentType::UNKNOWN;
            result.intent_name = "unknown";
            return result;
        }

        std::vector<std::string> tokens = tokenize(cleaned);
        if (tokens.empty()) {
            result.intent = MKIntentType::UNKNOWN;
            result.intent_name = "unknown";
            return result;
        }

        result.keywords = extractKeywords(tokens);

        // Priority 1: Check for commands
        if (isCommand(tokens, result)) return result;

        // Priority 2: Check for greetings
        if (isGreeting(tokens, result)) return result;

        // Priority 3: Check for questions
        if (isQuestion(tokens, cleaned, result)) return result;

        // Priority 4: Check for statements/facts
        if (isStatement(tokens, result)) return result;

        // Fallback: treat as unknown (possibly a question without markers)
        result.intent = MKIntentType::QUESTION;
        result.intent_name = "question";
        result.subject = tokens.size() > 0 ? tokens[0] : "";
        result.object = tokens.size() > 1 ? tokens[tokens.size() - 1] : "";
        result.confidence = 0.3f;
        return result;
    }

private:
    // ─────────────────────────────────────────
    //  INTENT DETECTION: COMMANDS
    // ─────────────────────────────────────────

    bool isCommand(const std::vector<std::string>& tokens, MKParsedQuery& result) {
        if (tokens.empty()) return false;

        // Check if first word is a command
        if (command_words.find(tokens[0]) != command_words.end()) {
            result.intent = MKIntentType::COMMAND;
            result.intent_name = "command";
            result.command_verb = tokens[0];
            result.confidence = 0.9f;

            // "tell me about X" pattern
            if (tokens[0] == "tell" && tokens.size() >= 4 &&
                tokens[1] == "me" && tokens[2] == "about") {
                result.command_verb = "tell_about";
                result.subject = joinTokens(tokens, 3, tokens.size());
                return true;
            }

            // "remember X is Y" or "learn X is Y" pattern
            if ((tokens[0] == "remember" || tokens[0] == "learn" || tokens[0] == "teach") &&
                tokens.size() >= 2) {
                // Everything after the command is a fact to parse
                std::vector<std::string> factTokens(tokens.begin() + 1, tokens.end());
                extractFactFromTokens(factTokens, result);
                return true;
            }

            // "show/list/dump" -> informational command
            if (tokens[0] == "show" || tokens[0] == "list" || tokens[0] == "dump") {
                if (tokens.size() >= 2) {
                    result.subject = joinTokens(tokens, 1, tokens.size());
                }
                return true;
            }

            // Simple commands: help, status, quit, exit, stats, rules
            if (tokens.size() >= 2) {
                result.subject = joinTokens(tokens, 1, tokens.size());
            }
            return true;
        }

        // Check for "forget about X" pattern
        if (tokens.size() >= 2 && tokens[0] == "forget") {
            result.intent = MKIntentType::COMMAND;
            result.intent_name = "command";
            result.command_verb = "forget";
            int start = (tokens.size() >= 3 && tokens[1] == "about") ? 2 : 1;
            result.subject = joinTokens(tokens, start, tokens.size());
            result.confidence = 0.9f;
            return true;
        }

        return false;
    }

    // ─────────────────────────────────────────
    //  INTENT DETECTION: GREETINGS
    // ─────────────────────────────────────────

    bool isGreeting(const std::vector<std::string>& tokens, MKParsedQuery& result) {
        if (tokens.empty()) return false;

        if (greeting_words.find(tokens[0]) != greeting_words.end()) {
            // Only treat as greeting if the sentence is short
            if (tokens.size() <= 4) {
                result.intent = MKIntentType::GREETING;
                result.intent_name = "greeting";
                result.subject = tokens[0];
                result.confidence = 0.85f;
                return true;
            }
        }
        return false;
    }

    // ─────────────────────────────────────────
    //  INTENT DETECTION: QUESTIONS
    // ─────────────────────────────────────────

    bool isQuestion(const std::vector<std::string>& tokens, const std::string& raw,
                    MKParsedQuery& result) {
        if (tokens.empty()) return false;

        bool hasQuestionMark = endsWithQuestion(raw);
        bool hasQuestionWord = startsWithQuestionWord(tokens);

        if (!hasQuestionMark && !hasQuestionWord) return false;

        result.intent = MKIntentType::QUESTION;
        result.intent_name = "question";
        result.confidence = hasQuestionMark ? 0.95f : 0.8f;

        if (hasQuestionWord) {
            result.question_word = tokens[0];
        }

        // Pattern: "what is X?" or "what are X?"
        if (tokens.size() >= 3 && (tokens[0] == "what" || tokens[0] == "who") &&
            linking_verbs.find(tokens[1]) != linking_verbs.end()) {
            result.subject = joinTokens(tokens, 2, tokens.size());
            result.relation = "is_a";
            result.confidence = 0.95f;
            return true;
        }

        // Pattern: "where is X?" or "where does X live?"
        if (tokens[0] == "where") {
            if (tokens.size() >= 3) {
                // "where is X" -> X lives_in ?
                if (linking_verbs.find(tokens[1]) != linking_verbs.end()) {
                    result.subject = joinTokens(tokens, 2, tokens.size());
                    result.relation = "lives_in";
                } else {
                    result.subject = joinTokens(tokens, 2, tokens.size());
                    result.relation = "located_in";
                }
            }
            return true;
        }

        // Pattern: "does X have Y?" or "does X like Y?"
        if ((tokens[0] == "does" || tokens[0] == "do" || tokens[0] == "did") &&
            tokens.size() >= 4) {
            // Find the verb
            result.subject = tokens[1];
            for (size_t i = 2; i < tokens.size(); i++) {
                auto relIt = verb_to_relation.find(tokens[i]);
                if (relIt != verb_to_relation.end()) {
                    result.relation = relIt->second;
                    if (i + 1 < tokens.size()) {
                        result.object = joinTokens(tokens, i + 1, tokens.size());
                    }
                    break;
                }
            }
            if (result.relation.empty() && tokens.size() >= 4) {
                result.relation = tokens[2];
                result.object = joinTokens(tokens, 3, tokens.size());
            }
            return true;
        }

        // Pattern: "can X do Y?"
        if (tokens[0] == "can" && tokens.size() >= 3) {
            result.subject = tokens[1];
            result.relation = "can";
            if (tokens.size() >= 4) {
                result.object = joinTokens(tokens, 2, tokens.size());
            }
            return true;
        }

        // Pattern: "how does X work?" "how many X?"
        if (tokens[0] == "how") {
            if (tokens.size() >= 3) {
                result.subject = joinTokens(tokens, 2, tokens.size());
                result.relation = "has_property";
            }
            return true;
        }

        // Pattern: "why is/does X Y?"
        if (tokens[0] == "why" && tokens.size() >= 3) {
            result.subject = joinTokens(tokens, 2, tokens.size());
            result.relation = "because";
            return true;
        }

        // Pattern: "when did/does/is X?"
        if (tokens[0] == "when" && tokens.size() >= 3) {
            result.subject = joinTokens(tokens, 2, tokens.size());
            result.relation = "time";
            return true;
        }

        // Pattern: "is X a Y?" or "is X Y?"
        if ((tokens[0] == "is" || tokens[0] == "are") && tokens.size() >= 3) {
            result.subject = tokens[1];
            result.relation = "is_a";
            // Skip "a" or "an" if present
            int objStart = 2;
            if (tokens.size() > 3 && (tokens[2] == "a" || tokens[2] == "an")) {
                objStart = 3;
            }
            result.object = joinTokens(tokens, objStart, tokens.size());
            return true;
        }

        // Generic question: extract subject from remaining words
        if (tokens.size() >= 2) {
            int start = hasQuestionWord ? 1 : 0;
            result.subject = joinTokens(tokens, start, tokens.size());
        }

        return true;
    }

    // ─────────────────────────────────────────
    //  INTENT DETECTION: STATEMENTS (FACTS)
    // ─────────────────────────────────────────

    bool isStatement(const std::vector<std::string>& tokens, MKParsedQuery& result) {
        if (tokens.size() < 3) return false;

        result.intent = MKIntentType::STATEMENT;
        result.intent_name = "statement";

        return extractFactFromTokens(tokens, result);
    }

    // Extract subject-relation-object from tokens
    bool extractFactFromTokens(const std::vector<std::string>& tokens, MKParsedQuery& result) {
        if (tokens.size() < 3) return false;

        // Pattern: "X is Y" / "X is a Y"
        for (size_t i = 1; i < tokens.size(); i++) {
            if (linking_verbs.find(tokens[i]) != linking_verbs.end()) {
                result.subject = joinTokens(tokens, 0, i);
                result.relation = "is_a";
                // Skip "a" or "an" after linking verb
                size_t objStart = i + 1;
                if (objStart < tokens.size() &&
                    (tokens[objStart] == "a" || tokens[objStart] == "an")) {
                    objStart++;
                }
                result.object = joinTokens(tokens, objStart, tokens.size());
                result.confidence = 0.85f;
                return true;
            }
        }

        // Pattern: "X has Y" / "X have Y"
        for (size_t i = 1; i < tokens.size(); i++) {
            if (possession_verbs.find(tokens[i]) != possession_verbs.end()) {
                result.subject = joinTokens(tokens, 0, i);
                result.relation = "has";
                size_t objStart = i + 1;
                if (objStart < tokens.size() &&
                    (tokens[objStart] == "a" || tokens[objStart] == "an")) {
                    objStart++;
                }
                result.object = joinTokens(tokens, objStart, tokens.size());
                result.confidence = 0.85f;
                return true;
            }
        }

        // Pattern: "X [verb] Y" - check verb_to_relation map
        for (size_t i = 1; i < tokens.size(); i++) {
            auto it = verb_to_relation.find(tokens[i]);
            if (it != verb_to_relation.end()) {
                result.subject = joinTokens(tokens, 0, i);
                result.relation = it->second;
                size_t objStart = i + 1;
                // Skip prepositions after certain verbs
                if (objStart < tokens.size() &&
                    (tokens[objStart] == "in" || tokens[objStart] == "at" ||
                     tokens[objStart] == "to" || tokens[objStart] == "the" ||
                     tokens[objStart] == "a")) {
                    objStart++;
                }
                result.object = joinTokens(tokens, objStart, tokens.size());
                result.confidence = 0.8f;
                return true;
            }
        }

        // Fallback: first word = subject, last word = object, middle = relation
        if (tokens.size() >= 3) {
            result.subject = tokens[0];
            result.relation = tokens[1];
            result.object = joinTokens(tokens, 2, tokens.size());
            result.confidence = 0.5f;
            return true;
        }

        return false;
    }

    // ─────────────────────────────────────────
    //  HELPER: JOIN TOKENS
    // ─────────────────────────────────────────

    std::string joinTokens(const std::vector<std::string>& tokens, size_t start, size_t end) {
        std::string result;
        for (size_t i = start; i < end && i < tokens.size(); i++) {
            if (!result.empty()) result += " ";
            result += tokens[i];
        }
        return result;
    }

public:
    // ─────────────────────────────────────────
    //  DEBUG & STATS
    // ─────────────────────────────────────────

    int parseCount() const { return total_parsed; }

    void printParsed(const MKParsedQuery& q) const {
        std::cout << "\n--- [PARSED QUERY] ---\n";
        std::cout << "  Raw: \"" << q.raw_input << "\"\n";
        std::cout << "  Intent: " << q.intent_name << " (conf=" << q.confidence << ")\n";
        if (!q.question_word.empty())
            std::cout << "  Question word: " << q.question_word << "\n";
        if (!q.command_verb.empty())
            std::cout << "  Command: " << q.command_verb << "\n";
        if (!q.subject.empty())
            std::cout << "  Subject: \"" << q.subject << "\"\n";
        if (!q.relation.empty())
            std::cout << "  Relation: \"" << q.relation << "\"\n";
        if (!q.object.empty())
            std::cout << "  Object: \"" << q.object << "\"\n";
        if (!q.keywords.empty()) {
            std::cout << "  Keywords: [";
            for (size_t i = 0; i < q.keywords.size(); i++) {
                if (i > 0) std::cout << ", ";
                std::cout << q.keywords[i];
            }
            std::cout << "]\n";
        }
        std::cout << "----------------------\n";
    }

    void printStats() const {
        std::cout << "\n--- [QUERY PARSER] ---\n";
        std::cout << "  Queries parsed: " << total_parsed << "\n";
        std::cout << "  Question words: " << question_words.size() << "\n";
        std::cout << "  Command words: " << command_words.size() << "\n";
        std::cout << "  Verb mappings: " << verb_to_relation.size() << "\n";
        std::cout << "----------------------\n";
    }
};

#endif // MK_QUERY_PARSER_CPP
