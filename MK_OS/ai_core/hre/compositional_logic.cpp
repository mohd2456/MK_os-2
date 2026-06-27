// ============================================================================
// MK OS — Compositional Logic Engine
// ============================================================================
// Handles questions MK was NEVER explicitly taught.
//
// How it works:
// - Breaks ANY statement into logical atoms (subject, predicate, object, mods)
// - Applies logical operations: AND, OR, NOT, IMPLIES, EXCEPT
// - Handles exceptions: "birds can fly EXCEPT penguins"
// - Handles negation: "cats are NOT reptiles"
// - Handles hypotheticals: "IF X is true THEN Y follows"
// - Handles quantifiers: "ALL cats have tails" vs "SOME cats are orange"
//
// Returns confidence with every evaluation:
//   certain / likely / unlikely / false / unknown
//
// Pure C++ STL. No external dependencies.
// ============================================================================

#ifndef MK_COMPOSITIONAL_LOGIC_CPP
#define MK_COMPOSITIONAL_LOGIC_CPP

#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <sstream>
#include <iostream>

// ============================================================================
// Enums & Constants
// ============================================================================

// Confidence levels for evaluation results
enum class Confidence {
    CERTAIN,    // 100% sure (direct fact in graph)
    LIKELY,     // 70-99% (inferred via rules)
    UNLIKELY,   // 10-30% (weak evidence against)
    FALSE,      // 0% (direct contradiction found)
    UNKNOWN     // No evidence either way
};

// Quantifier types
enum class Quantifier {
    ALL,        // Universal: "ALL cats..."
    SOME,       // Existential: "SOME cats..."
    NONE,       // Negated universal: "NO cats..."
    MOST,       // Approximate: "MOST birds..."
    UNSPECIFIED // No quantifier detected
};

// Logical operator for compound statements
enum class LogicOp {
    AND,        // Both must be true
    OR,         // Either can be true
    NOT,        // Negation
    IMPLIES,    // If A then B
    EXCEPT,     // A is true EXCEPT when B
    IFF         // If and only if
};

// ============================================================================
// Structures
// ============================================================================

// A single logical atom — the smallest unit of meaning
struct LogicalAtom {
    std::string subject;              // Who/what the statement is about
    std::string predicate;            // The relation/action
    std::string object;               // Target of the relation
    bool negated;                     // Is this statement negated?
    Quantifier quantifier;            // ALL/SOME/NONE/MOST
    std::vector<std::string> exceptions;  // EXCEPT these
    std::vector<std::string> conditions;  // IF these are true
    std::string rawText;              // Original text for reference

    LogicalAtom() : negated(false), quantifier(Quantifier::UNSPECIFIED) {}
};

// A compound statement (multiple atoms connected by logic)
struct CompoundStatement {
    std::vector<LogicalAtom> atoms;
    std::vector<LogicOp> operators;   // operators[i] connects atoms[i] to atoms[i+1]
};

// Evaluation result with evidence
struct EvalResult {
    Confidence confidence;
    bool truthValue;                  // Best guess: true or false
    std::string explanation;          // Why we think this
    std::vector<std::string> evidence;    // Supporting facts found
    std::vector<std::string> counterEvidence;  // Contradicting facts
};

// Exception rule stored in the database
struct ExceptionRule {
    std::string generalRule;          // "birds can fly"
    std::string exception;            // "penguin"
    std::string condition;            // Why (optional)
};

// Graph edge for querying (matches semantic_match.cpp interface)
struct LogicGraphEdge {
    std::string source;
    std::string relation;
    std::string target;
    float weight;
};

// ============================================================================
// MKCompositionalLogic — Main Class
// ============================================================================

class MKCompositionalLogic {
private:
    // Exception database: stores "X EXCEPT Y" overrides
    // Key = "subject|predicate|object", Value = list of exceptions
    std::map<std::string, std::vector<ExceptionRule>> exceptionDB;

    // Negation words that flip truth value
    std::set<std::string> negationWords;

    // Quantifier keywords
    std::map<std::string, Quantifier> quantifierWords;

    // Logic connector keywords
    std::map<std::string, LogicOp> connectorWords;

    // Implication rules learned from graph
    // "if X then Y" format
    std::vector<std::pair<LogicalAtom, LogicalAtom>> implications;

    // ========================================================================
    // Private: Initialization
    // ========================================================================

    void initializeKeywords() {
        // Words that negate a statement
        negationWords = {
            "not", "no", "never", "neither", "nor", "nobody",
            "nothing", "nowhere", "cannot", "can't", "won't",
            "don't", "doesn't", "didn't", "isn't", "aren't",
            "wasn't", "weren't", "hasn't", "haven't", "hadn't",
            "couldn't", "shouldn't", "wouldn't", "mustn't"
        };

        // Quantifier detection
        quantifierWords = {
            {"all", Quantifier::ALL},
            {"every", Quantifier::ALL},
            {"each", Quantifier::ALL},
            {"any", Quantifier::ALL},
            {"some", Quantifier::SOME},
            {"a few", Quantifier::SOME},
            {"several", Quantifier::SOME},
            {"many", Quantifier::MOST},
            {"most", Quantifier::MOST},
            {"no", Quantifier::NONE},
            {"none", Quantifier::NONE},
            {"zero", Quantifier::NONE}
        };

        // Logical connectors
        connectorWords = {
            {"and", LogicOp::AND},
            {"or", LogicOp::OR},
            {"but", LogicOp::EXCEPT},
            {"except", LogicOp::EXCEPT},
            {"unless", LogicOp::EXCEPT},
            {"however", LogicOp::EXCEPT},
            {"if", LogicOp::IMPLIES},
            {"then", LogicOp::IMPLIES},
            {"therefore", LogicOp::IMPLIES},
            {"because", LogicOp::IMPLIES}
        };
    }

    // ========================================================================
    // Private: Text Utilities
    // ========================================================================

    // Convert string to lowercase for comparison
    std::string toLower(const std::string& s) {
        std::string result = s;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

    // Split string into words
    std::vector<std::string> tokenize(const std::string& text) {
        std::vector<std::string> tokens;
        std::istringstream stream(text);
        std::string word;
        while (stream >> word) {
            // Strip punctuation from ends
            while (!word.empty() && std::ispunct(word.back())) word.pop_back();
            while (!word.empty() && std::ispunct(word.front())) word.erase(word.begin());
            if (!word.empty()) {
                tokens.push_back(toLower(word));
            }
        }
        return tokens;
    }

    // Check if a word is a negation word
    bool isNegation(const std::string& word) {
        return negationWords.count(toLower(word)) > 0;
    }

    // Detect if sentence contains negation
    bool detectNegation(const std::vector<std::string>& tokens) {
        for (const auto& token : tokens) {
            if (isNegation(token)) return true;
        }
        // Check for "n't" contractions
        for (const auto& token : tokens) {
            if (token.size() > 3 && token.substr(token.size() - 3) == "n't") {
                return true;
            }
        }
        return false;
    }

    // Detect quantifier in sentence
    Quantifier detectQuantifier(const std::vector<std::string>& tokens) {
        for (const auto& token : tokens) {
            auto it = quantifierWords.find(token);
            if (it != quantifierWords.end()) {
                return it->second;
            }
        }
        return Quantifier::UNSPECIFIED;
    }

    // Extract exceptions from "X except Y" patterns
    std::vector<std::string> extractExceptions(const std::vector<std::string>& tokens) {
        std::vector<std::string> exceptions;
        bool foundExcept = false;
        for (const auto& token : tokens) {
            if (token == "except" || token == "unless" || token == "but" ||
                token == "excluding") {
                foundExcept = true;
                continue;
            }
            if (foundExcept) {
                // Everything after "except" is an exception
                if (token != "for" && token != "that" && token != "the") {
                    exceptions.push_back(token);
                }
            }
        }
        return exceptions;
    }

    // Extract conditions from "if X then Y" patterns
    std::vector<std::string> extractConditions(const std::vector<std::string>& tokens) {
        std::vector<std::string> conditions;
        bool inCondition = false;
        for (size_t i = 0; i < tokens.size(); i++) {
            if (tokens[i] == "if" || tokens[i] == "when" || tokens[i] == "given") {
                inCondition = true;
                continue;
            }
            if (tokens[i] == "then" || tokens[i] == "," ) {
                inCondition = false;
                continue;
            }
            if (inCondition) {
                conditions.push_back(tokens[i]);
            }
        }
        return conditions;
    }

    // ========================================================================
    // Private: Graph Querying
    // ========================================================================

    // Check if a direct fact exists in the graph
    bool factExists(const std::string& subject, const std::string& relation,
                    const std::string& object,
                    const std::vector<LogicGraphEdge>& graph) {
        for (const auto& edge : graph) {
            if (edge.source == subject && edge.relation == relation &&
                edge.target == object) {
                return true;
            }
        }
        return false;
    }

    // Find all facts about a subject
    std::vector<LogicGraphEdge> factsAbout(const std::string& subject,
                                            const std::vector<LogicGraphEdge>& graph) {
        std::vector<LogicGraphEdge> results;
        for (const auto& edge : graph) {
            if (edge.source == subject || edge.target == subject) {
                results.push_back(edge);
            }
        }
        return results;
    }

    // Check inheritance: is subject a type of something that has this property?
    bool checkInheritance(const std::string& subject, const std::string& predicate,
                          const std::string& object,
                          const std::vector<LogicGraphEdge>& graph,
                          int depth = 0) {
        if (depth > 5) return false;  // Prevent infinite loops

        // Direct check
        if (factExists(subject, predicate, object, graph)) return true;

        // Find parent categories (is_a, type_of)
        for (const auto& edge : graph) {
            if (edge.source == subject &&
                (edge.relation == "is_a" || edge.relation == "type_of")) {
                // Check if parent has this property
                if (checkInheritance(edge.target, predicate, object, graph, depth + 1)) {
                    return true;
                }
            }
        }
        return false;
    }

    // ========================================================================
    // Private: Exception Checking
    // ========================================================================

    // Check if this specific case is an exception to a general rule
    bool isException(const std::string& subject, const std::string& predicate,
                     const std::string& object) {
        std::string key = subject + "|" + predicate + "|" + object;

        // Check direct exception
        for (const auto& pair : exceptionDB) {
            for (const auto& rule : pair.second) {
                if (rule.exception == subject) {
                    return true;
                }
            }
        }

        // Check if subject is listed in exceptions for this predicate+object
        std::string generalKey = "*|" + predicate + "|" + object;
        auto it = exceptionDB.find(generalKey);
        if (it != exceptionDB.end()) {
            for (const auto& rule : it->second) {
                if (rule.exception == subject) return true;
            }
        }

        return false;
    }

    // ========================================================================
    // Private: Confidence Calculation
    // ========================================================================

    Confidence calculateConfidence(bool directFact, bool inherited,
                                   bool hasException, int evidenceCount,
                                   int counterCount) {
        if (hasException) return Confidence::FALSE;
        if (directFact) return Confidence::CERTAIN;
        if (inherited && counterCount == 0) return Confidence::LIKELY;
        if (evidenceCount > counterCount && evidenceCount > 0) return Confidence::LIKELY;
        if (counterCount > evidenceCount && counterCount > 0) return Confidence::UNLIKELY;
        if (evidenceCount == 0 && counterCount == 0) return Confidence::UNKNOWN;
        return Confidence::UNKNOWN;
    }

public:
    // ========================================================================
    // Constructor
    // ========================================================================

    MKCompositionalLogic() {
        initializeKeywords();
    }

    // ========================================================================
    // Core API: Decompose
    // ========================================================================

    // Parse a natural language statement into a logical atom
    // "All birds can fly except penguins" →
    //   { subject: "birds", predicate: "can", object: "fly",
    //     negated: false, quantifier: ALL, exceptions: ["penguins"] }
    LogicalAtom decompose(const std::string& sentence) {
        LogicalAtom atom;
        atom.rawText = sentence;

        std::vector<std::string> tokens = tokenize(sentence);
        if (tokens.empty()) return atom;

        // Detect negation
        atom.negated = detectNegation(tokens);

        // Detect quantifier
        atom.quantifier = detectQuantifier(tokens);

        // Extract exceptions
        atom.exceptions = extractExceptions(tokens);

        // Extract conditions
        atom.conditions = extractConditions(tokens);

        // Remove negation words, quantifiers, exceptions for core parsing
        std::vector<std::string> coreTokens;
        bool skipUntilComma = false;
        for (size_t i = 0; i < tokens.size(); i++) {
            if (tokens[i] == "except" || tokens[i] == "unless") {
                break;  // Everything after exception marker was already captured
            }
            if (isNegation(tokens[i])) continue;
            if (quantifierWords.count(tokens[i]) > 0) continue;
            if (tokens[i] == "if" || tokens[i] == "when") {
                skipUntilComma = true;
                continue;
            }
            if (skipUntilComma) {
                if (tokens[i] == "then" || tokens[i] == ",") {
                    skipUntilComma = false;
                }
                continue;
            }
            coreTokens.push_back(tokens[i]);
        }

        // Simple SVO (Subject-Verb-Object) extraction from core tokens
        // Subject = first noun-like token(s)
        // Predicate = verb-like token (is, are, can, has, etc.)
        // Object = remaining tokens
        std::set<std::string> verbLike = {
            "is", "are", "was", "were", "can", "has", "have", "had",
            "will", "would", "could", "should", "do", "does", "did",
            "make", "makes", "made"
        };

        int verbPos = -1;
        for (size_t i = 0; i < coreTokens.size(); i++) {
            if (verbLike.count(coreTokens[i]) > 0) {
                verbPos = (int)i;
                break;
            }
        }

        if (verbPos > 0) {
            // Subject = everything before verb
            for (int i = 0; i < verbPos; i++) {
                if (!atom.subject.empty()) atom.subject += " ";
                atom.subject += coreTokens[i];
            }
            // Predicate = the verb
            atom.predicate = coreTokens[verbPos];
            // Object = everything after verb
            for (size_t i = verbPos + 1; i < coreTokens.size(); i++) {
                if (!atom.object.empty()) atom.object += " ";
                atom.object += coreTokens[i];
            }
        } else if (!coreTokens.empty()) {
            // No verb found — use first token as subject, rest as object
            atom.subject = coreTokens[0];
            for (size_t i = 1; i < coreTokens.size(); i++) {
                if (!atom.object.empty()) atom.object += " ";
                atom.object += coreTokens[i];
            }
        }

        return atom;
    }

    // ========================================================================
    // Core API: Evaluate
    // ========================================================================

    // Evaluate whether a logical atom is true, false, or unknown
    // given the current knowledge graph
    EvalResult evaluate(const LogicalAtom& atom,
                        const std::vector<LogicGraphEdge>& graph) {
        EvalResult result;
        result.truthValue = false;
        result.confidence = Confidence::UNKNOWN;

        if (atom.subject.empty()) {
            result.explanation = "Cannot evaluate: no subject found";
            return result;
        }

        // Step 1: Check for exceptions
        bool hasException = isException(atom.subject, atom.predicate, atom.object);
        if (hasException) {
            result.truthValue = false;
            result.confidence = Confidence::FALSE;
            result.explanation = atom.subject + " is an exception to the rule '" +
                                atom.predicate + " " + atom.object + "'";
            result.counterEvidence.push_back("Exception rule applies");
            // If the atom was negated, flip the result
            if (atom.negated) {
                result.truthValue = true;
                result.confidence = Confidence::CERTAIN;
            }
            return result;
        }

        // Step 2: Direct fact lookup
        bool directFact = factExists(atom.subject, atom.predicate, atom.object, graph);
        if (directFact) {
            result.evidence.push_back(atom.subject + "|" + atom.predicate + "|" + atom.object);
        }

        // Step 3: Inheritance check (is subject a type of something with this property?)
        bool inherited = false;
        if (!directFact) {
            inherited = checkInheritance(atom.subject, atom.predicate, atom.object, graph);
            if (inherited) {
                result.evidence.push_back("Inherited from parent category");
            }
        }

        // Step 4: Check for contradicting facts
        // Look for negated versions of this fact
        bool contradicted = factExists(atom.subject, "not_" + atom.predicate,
                                       atom.object, graph);
        if (contradicted) {
            result.counterEvidence.push_back("Contradicting fact found: not_" +
                                             atom.predicate);
        }

        // Step 5: Quantifier handling
        if (atom.quantifier == Quantifier::ALL) {
            // "ALL X have Y" — check if any exception exists
            // If we find even one counter-example, ALL is false
            // (This would need iteration over all instances, simplified here)
            if (contradicted) {
                result.counterEvidence.push_back("Not ALL — exception exists");
            }
        } else if (atom.quantifier == Quantifier::NONE) {
            // "NO X have Y" — if we find even one positive, NONE is false
            if (directFact || inherited) {
                result.counterEvidence.push_back("At least one instance found");
            }
        }

        // Step 6: Calculate confidence
        result.confidence = calculateConfidence(
            directFact, inherited, hasException,
            (int)result.evidence.size(), (int)result.counterEvidence.size());

        // Step 7: Determine truth value
        result.truthValue = (result.confidence == Confidence::CERTAIN ||
                            result.confidence == Confidence::LIKELY);

        // Step 8: Handle negation (flip result)
        if (atom.negated) {
            result.truthValue = !result.truthValue;
            // Flip confidence appropriately
            if (result.confidence == Confidence::CERTAIN)
                result.confidence = Confidence::FALSE;
            else if (result.confidence == Confidence::FALSE)
                result.confidence = Confidence::CERTAIN;
            else if (result.confidence == Confidence::LIKELY)
                result.confidence = Confidence::UNLIKELY;
            else if (result.confidence == Confidence::UNLIKELY)
                result.confidence = Confidence::LIKELY;
        }

        // Step 9: Build explanation
        result.explanation = buildExplanation(atom, result);

        return result;
    }

    // ========================================================================
    // Core API: Explain
    // ========================================================================

    // Explain WHY a statement is true/false with an evidence chain
    std::string explain(const LogicalAtom& atom,
                       const std::vector<LogicGraphEdge>& graph) {
        EvalResult result = evaluate(atom, graph);
        return result.explanation;
    }

    // ========================================================================
    // Core API: Add Exception
    // ========================================================================

    // Register an exception: "X except Y"
    void addException(const std::string& generalSubject,
                      const std::string& predicate,
                      const std::string& object,
                      const std::string& exceptionEntity,
                      const std::string& reason = "") {
        ExceptionRule rule;
        rule.generalRule = generalSubject + " " + predicate + " " + object;
        rule.exception = exceptionEntity;
        rule.condition = reason;

        std::string key = "*|" + predicate + "|" + object;
        exceptionDB[key].push_back(rule);

        std::cout << "[CompLogic] Added exception: " << rule.generalRule
                  << " EXCEPT " << exceptionEntity << std::endl;
    }

    // ========================================================================
    // Core API: Evaluate Compound Statement
    // ========================================================================

    // Evaluate compound statements with AND/OR/IMPLIES
    EvalResult evaluateCompound(const CompoundStatement& stmt,
                                const std::vector<LogicGraphEdge>& graph) {
        if (stmt.atoms.empty()) {
            EvalResult empty;
            empty.confidence = Confidence::UNKNOWN;
            empty.truthValue = false;
            empty.explanation = "Empty statement";
            return empty;
        }

        // Evaluate first atom
        EvalResult cumulative = evaluate(stmt.atoms[0], graph);

        // Apply operators sequentially
        for (size_t i = 0; i < stmt.operators.size() && i + 1 < stmt.atoms.size(); i++) {
            EvalResult next = evaluate(stmt.atoms[i + 1], graph);

            switch (stmt.operators[i]) {
                case LogicOp::AND:
                    // Both must be true
                    cumulative.truthValue = cumulative.truthValue && next.truthValue;
                    if (!next.truthValue) {
                        cumulative.confidence = next.confidence;
                        cumulative.explanation += " AND " + next.explanation;
                    }
                    break;

                case LogicOp::OR:
                    // Either can be true
                    cumulative.truthValue = cumulative.truthValue || next.truthValue;
                    if (next.truthValue && !cumulative.truthValue) {
                        cumulative.confidence = next.confidence;
                    }
                    cumulative.explanation += " OR " + next.explanation;
                    break;

                case LogicOp::NOT:
                    // Negate the next
                    next.truthValue = !next.truthValue;
                    cumulative.truthValue = cumulative.truthValue && next.truthValue;
                    cumulative.explanation += " NOT " + next.explanation;
                    break;

                case LogicOp::IMPLIES:
                    // If first is true, second must be true
                    // If first is false, implication is vacuously true
                    if (cumulative.truthValue && !next.truthValue) {
                        cumulative.truthValue = false;
                        cumulative.confidence = Confidence::FALSE;
                        cumulative.explanation += " IMPLIES " + next.explanation +
                                                  " (antecedent true but consequent false)";
                    } else {
                        cumulative.truthValue = true;
                        cumulative.explanation += " IMPLIES " + next.explanation;
                    }
                    break;

                case LogicOp::EXCEPT:
                    // First is true EXCEPT when second is true
                    if (next.truthValue) {
                        cumulative.truthValue = false;
                        cumulative.confidence = Confidence::FALSE;
                        cumulative.explanation += " EXCEPT " + next.explanation +
                                                  " (exception applies)";
                    }
                    break;

                case LogicOp::IFF:
                    // Both must have same truth value
                    cumulative.truthValue = (cumulative.truthValue == next.truthValue);
                    cumulative.explanation += " IFF " + next.explanation;
                    break;
            }
        }

        return cumulative;
    }

    // ========================================================================
    // Core API: Parse Compound from Natural Language
    // ========================================================================

    // Parse a complex sentence into a compound statement
    CompoundStatement parseCompound(const std::string& sentence) {
        CompoundStatement stmt;

        // Split on logical connectors
        std::vector<std::string> tokens = tokenize(sentence);
        std::string current;
        LogicOp pendingOp = LogicOp::AND;
        bool firstAtom = true;

        for (size_t i = 0; i < tokens.size(); i++) {
            auto it = connectorWords.find(tokens[i]);
            if (it != connectorWords.end() && !current.empty()) {
                // Found a connector — save current atom
                LogicalAtom atom = decompose(current);
                stmt.atoms.push_back(atom);
                if (!firstAtom) {
                    stmt.operators.push_back(pendingOp);
                }
                pendingOp = it->second;
                current.clear();
                firstAtom = false;
            } else {
                if (!current.empty()) current += " ";
                current += tokens[i];
            }
        }

        // Don't forget the last atom
        if (!current.empty()) {
            LogicalAtom atom = decompose(current);
            stmt.atoms.push_back(atom);
            if (!firstAtom) {
                stmt.operators.push_back(pendingOp);
            }
        }

        return stmt;
    }

    // ========================================================================
    // Utility: Confidence to String
    // ========================================================================

    std::string confidenceToString(Confidence c) {
        switch (c) {
            case Confidence::CERTAIN: return "certain";
            case Confidence::LIKELY: return "likely";
            case Confidence::UNLIKELY: return "unlikely";
            case Confidence::FALSE: return "false";
            case Confidence::UNKNOWN: return "unknown";
        }
        return "unknown";
    }

private:
    // ========================================================================
    // Private: Build Explanation String
    // ========================================================================

    std::string buildExplanation(const LogicalAtom& atom, const EvalResult& result) {
        std::string explanation;

        // Start with the claim
        explanation = "\"" + atom.rawText + "\" → " +
                     confidenceToString(result.confidence) + "\n";

        // Evidence
        if (!result.evidence.empty()) {
            explanation += "  Evidence: ";
            for (size_t i = 0; i < result.evidence.size(); i++) {
                if (i > 0) explanation += ", ";
                explanation += result.evidence[i];
            }
            explanation += "\n";
        }

        // Counter-evidence
        if (!result.counterEvidence.empty()) {
            explanation += "  Against: ";
            for (size_t i = 0; i < result.counterEvidence.size(); i++) {
                if (i > 0) explanation += ", ";
                explanation += result.counterEvidence[i];
            }
            explanation += "\n";
        }

        // Exceptions noted
        if (!atom.exceptions.empty()) {
            explanation += "  Exceptions: ";
            for (size_t i = 0; i < atom.exceptions.size(); i++) {
                if (i > 0) explanation += ", ";
                explanation += atom.exceptions[i];
            }
            explanation += "\n";
        }

        return explanation;
    }
};

#endif // MK_COMPOSITIONAL_LOGIC_CPP
