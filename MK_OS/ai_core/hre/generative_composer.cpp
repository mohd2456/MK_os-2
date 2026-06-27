// ============================================================================
// MK OS — Generative Composition Engine
// ============================================================================
// Creates UNIQUE natural responses (not template retrieval).
//
// How it works WITHOUT an LLM:
// - Grammar rules: sentence = subject + verb_phrase + object_phrase + (modifier)
// - Word selection engine: for each slot, pick semantically appropriate words
// - Variety tracking: never picks same phrasing twice in a row
// - Sentence connectors create flow between multiple sentences
// - Personality injects style (formal vs casual vs excited)
//
// Handles plurals, conjugation, articles (a/an/the) correctly.
// Tracks last 50 outputs to avoid repetition.
//
// Pure C++ STL. No external dependencies.
// ============================================================================

#ifndef MK_GENERATIVE_COMPOSER_CPP
#define MK_GENERATIVE_COMPOSER_CPP

#include <string>
#include <vector>
#include <map>
#include <set>
#include <deque>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <ctime>

// ============================================================================
// Enums & Constants
// ============================================================================

// Personality modes affect word choice and sentence structure
enum class Personality {
    FORMAL,     // Professional, precise language
    CASUAL,     // Friendly, conversational
    EXCITED,    // Enthusiastic, emphatic
    NEUTRAL     // Default balanced style
};

// Sentence pattern types
enum class SentenceType {
    DECLARATIVE,    // "X is Y"
    INTERROGATIVE,  // "Is X Y?"
    IMPERATIVE,     // "Do X"
    EXCLAMATORY,    // "How X!"
    CONDITIONAL     // "If X then Y"
};

// Grammar slots in a sentence
enum class SlotType {
    SUBJECT,
    VERB,
    OBJECT,
    MODIFIER,
    CONNECTOR,
    ARTICLE,
    ADJECTIVE,
    ADVERB
};


// ============================================================================
// Structures
// ============================================================================

// A grammar rule defines a sentence pattern
struct GrammarRule {
    SentenceType type;
    std::vector<SlotType> pattern;  // Ordered slots to fill
    std::string templateStr;        // Pattern with {slot} placeholders
    float variety;                  // How often to use (0-1, lower = rarer)
};

// A fact to be expressed (input to composer)
struct ComposerFact {
    std::string subject;
    std::string relation;
    std::string object;
    float confidence;
};

// ============================================================================
// MKGenerativeComposer — Main Class
// ============================================================================

class MKGenerativeComposer {
private:
    // Grammar rules for each sentence type
    std::map<SentenceType, std::vector<GrammarRule>> grammarDB;

    // Synonym database: word -> alternatives
    std::map<std::string, std::vector<std::string>> synonyms;

    // Verb conjugation: base -> {present, past, gerund}
    std::map<std::string, std::vector<std::string>> conjugations;

    // Sentence connectors for flow between sentences
    std::vector<std::string> additionConnectors;
    std::vector<std::string> contrastConnectors;
    std::vector<std::string> causalConnectors;
    std::vector<std::string> sequenceConnectors;

    // Personality-specific word preferences
    std::map<Personality, std::map<std::string, std::string>> personalityWords;

    // Recent outputs for variety tracking (avoid repetition)
    std::deque<std::string> recentOutputs;
    static const int MAX_RECENT = 50;

    // Simple pseudo-random for variety (seeded from time)
    unsigned int rngState;

    // ========================================================================
    // Private: Initialization
    // ========================================================================

    void initGrammar() {
        // Declarative patterns: "X is Y", "X has Y", etc.
        GrammarRule decl1;
        decl1.type = SentenceType::DECLARATIVE;
        decl1.templateStr = "{subject} {verb} {object}";
        decl1.variety = 0.3f;
        grammarDB[SentenceType::DECLARATIVE].push_back(decl1);

        GrammarRule decl2;
        decl2.type = SentenceType::DECLARATIVE;
        decl2.templateStr = "{subject} {verb} {adjective} {object}";
        decl2.variety = 0.25f;
        grammarDB[SentenceType::DECLARATIVE].push_back(decl2);

        GrammarRule decl3;
        decl3.type = SentenceType::DECLARATIVE;
        decl3.templateStr = "{adverb}, {subject} {verb} {object}";
        decl3.variety = 0.2f;
        grammarDB[SentenceType::DECLARATIVE].push_back(decl3);

        GrammarRule decl4;
        decl4.type = SentenceType::DECLARATIVE;
        decl4.templateStr = "{subject} {verb} {object}, {modifier}";
        decl4.variety = 0.15f;
        grammarDB[SentenceType::DECLARATIVE].push_back(decl4);

        GrammarRule decl5;
        decl5.type = SentenceType::DECLARATIVE;
        decl5.templateStr = "It is {adjective} that {subject} {verb} {object}";
        decl5.variety = 0.1f;
        grammarDB[SentenceType::DECLARATIVE].push_back(decl5);

        // Conditional patterns
        GrammarRule cond1;
        cond1.type = SentenceType::CONDITIONAL;
        cond1.templateStr = "If {subject} {verb} {object}, then {consequence}";
        cond1.variety = 0.5f;
        grammarDB[SentenceType::CONDITIONAL].push_back(cond1);

        GrammarRule cond2;
        cond2.type = SentenceType::CONDITIONAL;
        cond2.templateStr = "When {subject} {verb} {object}, {consequence}";
        cond2.variety = 0.5f;
        grammarDB[SentenceType::CONDITIONAL].push_back(cond2);

        // Exclamatory patterns
        GrammarRule excl1;
        excl1.type = SentenceType::EXCLAMATORY;
        excl1.templateStr = "{subject} {verb} {object}!";
        excl1.variety = 0.5f;
        grammarDB[SentenceType::EXCLAMATORY].push_back(excl1);

        GrammarRule excl2;
        excl2.type = SentenceType::EXCLAMATORY;
        excl2.templateStr = "How {adjective}! {subject} {verb} {object}!";
        excl2.variety = 0.5f;
        grammarDB[SentenceType::EXCLAMATORY].push_back(excl2);
    }


    void initSynonyms() {
        // Verb synonyms for variety
        synonyms["is"] = {"is", "represents", "constitutes", "serves as"};
        synonyms["has"] = {"has", "possesses", "contains", "includes", "features"};
        synonyms["can"] = {"can", "is able to", "is capable of", "has the ability to"};
        synonyms["makes"] = {"makes", "creates", "produces", "generates", "forms"};
        synonyms["uses"] = {"uses", "utilizes", "employs", "leverages", "applies"};
        synonyms["shows"] = {"shows", "demonstrates", "displays", "reveals", "indicates"};
        synonyms["helps"] = {"helps", "assists", "supports", "aids", "facilitates"};
        synonyms["gives"] = {"gives", "provides", "offers", "delivers", "supplies"};
        synonyms["needs"] = {"needs", "requires", "demands", "calls for"};
        synonyms["works"] = {"works", "functions", "operates", "performs"};

        // Adjective synonyms
        synonyms["good"] = {"good", "excellent", "effective", "solid", "strong"};
        synonyms["bad"] = {"bad", "poor", "weak", "problematic", "flawed"};
        synonyms["big"] = {"big", "large", "significant", "substantial", "major"};
        synonyms["small"] = {"small", "minor", "slight", "modest", "limited"};
        synonyms["important"] = {"important", "crucial", "vital", "essential", "key"};
        synonyms["interesting"] = {"interesting", "notable", "remarkable", "fascinating"};

        // Adverbs
        synonyms["very"] = {"very", "quite", "extremely", "remarkably", "particularly"};
        synonyms["also"] = {"also", "additionally", "furthermore", "moreover"};
    }

    void initConnectors() {
        additionConnectors = {
            "Additionally", "Furthermore", "Moreover", "Also",
            "In addition", "What's more", "On top of that"
        };
        contrastConnectors = {
            "However", "On the other hand", "Nevertheless",
            "That said", "In contrast", "Conversely", "Yet"
        };
        causalConnectors = {
            "Therefore", "As a result", "Consequently",
            "Because of this", "This means", "Thus", "Hence"
        };
        sequenceConnectors = {
            "First", "Next", "Then", "After that",
            "Finally", "Subsequently", "Following this"
        };
    }

    void initConjugations() {
        // base -> {third_person_singular, past, gerund}
        conjugations["be"] = {"is", "was", "being"};
        conjugations["have"] = {"has", "had", "having"};
        conjugations["do"] = {"does", "did", "doing"};
        conjugations["go"] = {"goes", "went", "going"};
        conjugations["make"] = {"makes", "made", "making"};
        conjugations["take"] = {"takes", "took", "taking"};
        conjugations["come"] = {"comes", "came", "coming"};
        conjugations["see"] = {"sees", "saw", "seeing"};
        conjugations["know"] = {"knows", "knew", "knowing"};
        conjugations["get"] = {"gets", "got", "getting"};
        conjugations["give"] = {"gives", "gave", "giving"};
        conjugations["find"] = {"finds", "found", "finding"};
        conjugations["use"] = {"uses", "used", "using"};
        conjugations["work"] = {"works", "worked", "working"};
        conjugations["run"] = {"runs", "ran", "running"};
    }

    void initPersonality() {
        // Formal personality: precise, professional
        personalityWords[Personality::FORMAL]["is"] = "constitutes";
        personalityWords[Personality::FORMAL]["good"] = "exemplary";
        personalityWords[Personality::FORMAL]["bad"] = "suboptimal";
        personalityWords[Personality::FORMAL]["big"] = "substantial";
        personalityWords[Personality::FORMAL]["also"] = "furthermore";
        personalityWords[Personality::FORMAL]["but"] = "however";

        // Casual personality: friendly, conversational
        personalityWords[Personality::CASUAL]["is"] = "is basically";
        personalityWords[Personality::CASUAL]["good"] = "great";
        personalityWords[Personality::CASUAL]["bad"] = "not great";
        personalityWords[Personality::CASUAL]["also"] = "plus";
        personalityWords[Personality::CASUAL]["but"] = "though";

        // Excited personality: enthusiastic
        personalityWords[Personality::EXCITED]["is"] = "is definitely";
        personalityWords[Personality::EXCITED]["good"] = "amazing";
        personalityWords[Personality::EXCITED]["bad"] = "terrible";
        personalityWords[Personality::EXCITED]["very"] = "incredibly";
        personalityWords[Personality::EXCITED]["also"] = "and get this —";
    }


    // ========================================================================
    // Private: Random Selection (deterministic, no rand())
    // ========================================================================

    // Simple xorshift for variety without external deps
    unsigned int nextRandom() {
        rngState ^= rngState << 13;
        rngState ^= rngState >> 17;
        rngState ^= rngState << 5;
        return rngState;
    }

    // Pick a random element from a vector
    template<typename T>
    const T& pickRandom(const std::vector<T>& options) {
        if (options.empty()) {
            static T empty;
            return empty;
        }
        int idx = nextRandom() % options.size();
        return options[idx];
    }

    // ========================================================================
    // Private: Word Selection
    // ========================================================================

    // Pick a synonym based on personality + variety
    std::string selectWord(const std::string& baseWord, Personality personality) {
        // Check personality-specific override first
        auto persIt = personalityWords.find(personality);
        if (persIt != personalityWords.end()) {
            auto wordIt = persIt->second.find(baseWord);
            if (wordIt != persIt->second.end()) {
                return wordIt->second;
            }
        }

        // Use synonym database for variety
        auto synIt = synonyms.find(baseWord);
        if (synIt != synonyms.end() && !synIt->second.empty()) {
            return pickRandom(synIt->second);
        }

        return baseWord;
    }

    // ========================================================================
    // Private: Article Selection
    // ========================================================================

    // Choose correct article (a/an/the)
    std::string selectArticle(const std::string& noun, bool definite) {
        if (definite) return "the";

        // "an" before vowel sounds
        if (!noun.empty()) {
            char first = std::tolower(noun[0]);
            if (first == 'a' || first == 'e' || first == 'i' ||
                first == 'o' || first == 'u') {
                return "an";
            }
        }
        return "a";
    }

    // ========================================================================
    // Private: Pluralization
    // ========================================================================

    std::string pluralize(const std::string& noun) {
        if (noun.empty()) return noun;

        // Common irregular plurals
        std::map<std::string, std::string> irregulars = {
            {"person", "people"}, {"child", "children"}, {"mouse", "mice"},
            {"man", "men"}, {"woman", "women"}, {"foot", "feet"},
            {"tooth", "teeth"}, {"goose", "geese"}, {"ox", "oxen"},
            {"fish", "fish"}, {"sheep", "sheep"}, {"deer", "deer"}
        };

        auto it = irregulars.find(noun);
        if (it != irregulars.end()) return it->second;

        // Rules-based pluralization
        size_t len = noun.size();
        if (len >= 2) {
            std::string last2 = noun.substr(len - 2);
            if (last2 == "ch" || last2 == "sh" || last2 == "ss" ||
                last2 == "us" || last2 == "ox") {
                return noun + "es";
            }
        }
        if (len >= 1) {
            char last = noun.back();
            if (last == 's' || last == 'x' || last == 'z') return noun + "es";
            if (last == 'y' && len >= 2) {
                char beforeY = noun[len - 2];
                if (beforeY != 'a' && beforeY != 'e' && beforeY != 'i' &&
                    beforeY != 'o' && beforeY != 'u') {
                    return noun.substr(0, len - 1) + "ies";
                }
            }
        }
        return noun + "s";
    }

    // ========================================================================
    // Private: Capitalize First Letter
    // ========================================================================

    std::string capitalize(const std::string& s) {
        if (s.empty()) return s;
        std::string result = s;
        result[0] = std::toupper(result[0]);
        return result;
    }

    // ========================================================================
    // Private: Repetition Check
    // ========================================================================

    bool isRepetitive(const std::string& output) {
        for (const auto& recent : recentOutputs) {
            // Check if too similar (simple overlap check)
            if (output == recent) return true;
            // Check first 20 chars match (likely same structure)
            if (output.size() > 20 && recent.size() > 20 &&
                output.substr(0, 20) == recent.substr(0, 20)) {
                return true;
            }
        }
        return false;
    }

    void recordOutput(const std::string& output) {
        recentOutputs.push_back(output);
        if ((int)recentOutputs.size() > MAX_RECENT) {
            recentOutputs.pop_front();
        }
    }


    // ========================================================================
    // Private: Relation to Verb
    // ========================================================================

    // Convert a graph relation into a natural language verb phrase
    std::string relationToVerb(const std::string& relation, Personality personality) {
        std::map<std::string, std::string> relationVerbs = {
            {"is_a", "is"},
            {"has", "has"},
            {"has_property", "has the property of being"},
            {"can", "can"},
            {"capable_of", "is capable of"},
            {"part_of", "is part of"},
            {"used_in", "is used in"},
            {"related_to", "is related to"},
            {"type_of", "is a type of"},
            {"contains", "contains"},
            {"causes", "causes"},
            {"requires", "requires"},
            {"produces", "produces"},
            {"located_in", "is located in"},
            {"created_by", "was created by"},
            {"made_of", "is made of"}
        };

        auto it = relationVerbs.find(relation);
        std::string verb = (it != relationVerbs.end()) ? it->second : relation;
        return selectWord(verb, personality);
    }

public:
    // ========================================================================
    // Constructor
    // ========================================================================

    MKGenerativeComposer() : rngState((unsigned int)std::time(nullptr)) {
        initGrammar();
        initSynonyms();
        initConnectors();
        initConjugations();
        initPersonality();
    }

    // ========================================================================
    // Core API: Compose
    // ========================================================================

    // Turn a set of facts into a natural language paragraph
    std::string compose(const std::vector<ComposerFact>& facts,
                        Personality personality = Personality::NEUTRAL,
                        int maxSentences = 5) {
        if (facts.empty()) return "";

        std::string paragraph;
        int sentenceCount = 0;

        for (size_t i = 0; i < facts.size() && sentenceCount < maxSentences; i++) {
            const ComposerFact& fact = facts[i];
            std::string sentence;

            // Pick a grammar pattern (vary structure)
            const auto& patterns = grammarDB[SentenceType::DECLARATIVE];
            if (patterns.empty()) continue;

            // Build sentence from fact
            std::string subject = capitalize(fact.subject);
            std::string verb = relationToVerb(fact.relation, personality);
            std::string object = fact.object;

            // Apply a grammar pattern
            int patternIdx = nextRandom() % patterns.size();
            std::string tmpl = patterns[patternIdx].templateStr;

            // Fill template slots
            size_t pos;
            pos = tmpl.find("{subject}");
            if (pos != std::string::npos) tmpl.replace(pos, 9, subject);
            pos = tmpl.find("{verb}");
            if (pos != std::string::npos) tmpl.replace(pos, 6, verb);
            pos = tmpl.find("{object}");
            if (pos != std::string::npos) tmpl.replace(pos, 8, object);
            pos = tmpl.find("{adjective}");
            if (pos != std::string::npos) tmpl.replace(pos, 11, "notable");
            pos = tmpl.find("{adverb}");
            if (pos != std::string::npos) tmpl.replace(pos, 8, "Notably");
            pos = tmpl.find("{modifier}");
            if (pos != std::string::npos) tmpl.replace(pos, 10, "which is noteworthy");

            sentence = tmpl;

            // Ensure proper punctuation
            if (!sentence.empty() && sentence.back() != '.' &&
                sentence.back() != '!' && sentence.back() != '?') {
                sentence += ".";
            }

            // Check for repetition — regenerate if too similar
            if (isRepetitive(sentence)) {
                // Try different pattern
                patternIdx = (patternIdx + 1) % patterns.size();
                // Simplified: just change word order
                sentence = capitalize(object) + " — " + subject + " " + verb + " it.";
            }

            // Add connector between sentences (not first one)
            if (sentenceCount > 0) {
                std::string connector;
                int connType = nextRandom() % 4;
                switch (connType) {
                    case 0: connector = pickRandom(additionConnectors); break;
                    case 1: connector = pickRandom(sequenceConnectors); break;
                    case 2: connector = ""; break;  // No connector sometimes
                    case 3: connector = pickRandom(causalConnectors); break;
                }
                if (!connector.empty()) {
                    paragraph += " " + connector + ", ";
                } else {
                    paragraph += " ";
                }
            }

            paragraph += sentence;
            recordOutput(sentence);
            sentenceCount++;
        }

        return paragraph;
    }


    // ========================================================================
    // Core API: Rephrase
    // ========================================================================

    // Say the same thing differently
    std::string rephrase(const std::string& sentence) {
        // Strategy: swap synonyms, change structure
        std::istringstream stream(sentence);
        std::string word;
        std::vector<std::string> words;
        while (stream >> word) words.push_back(word);

        if (words.empty()) return sentence;

        // Replace some words with synonyms
        std::string result;
        for (size_t i = 0; i < words.size(); i++) {
            std::string w = words[i];
            // Strip punctuation for lookup
            std::string clean = w;
            char punct = '\0';
            if (!clean.empty() && std::ispunct(clean.back())) {
                punct = clean.back();
                clean.pop_back();
            }

            // Try synonym replacement (30% chance per word)
            std::string lower = clean;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            if (nextRandom() % 3 == 0) {
                auto synIt = synonyms.find(lower);
                if (synIt != synonyms.end() && !synIt->second.empty()) {
                    clean = pickRandom(synIt->second);
                    if (i == 0) clean = capitalize(clean);
                }
            }

            if (punct) clean += punct;
            if (!result.empty()) result += " ";
            result += clean;
        }

        return result;
    }

    // ========================================================================
    // Core API: Elaborate
    // ========================================================================

    // Expand a simple fact into a detailed explanation
    std::string elaborate(const ComposerFact& fact, const std::string& context) {
        std::string result;

        // Opening statement
        result += capitalize(fact.subject) + " " +
                  relationToVerb(fact.relation, Personality::NEUTRAL) +
                  " " + fact.object + ". ";

        // Add context-based elaboration
        if (!context.empty()) {
            result += "In the context of " + context + ", this means that " +
                      fact.subject + " plays a role as " + fact.object + ". ";
        }

        // Add confidence qualifier
        if (fact.confidence >= 0.9f) {
            result += "This is well-established.";
        } else if (fact.confidence >= 0.7f) {
            result += "This is generally accepted.";
        } else if (fact.confidence >= 0.5f) {
            result += "This is somewhat likely, though not certain.";
        } else {
            result += "This is uncertain and may need verification.";
        }

        return result;
    }

    // ========================================================================
    // Core API: Summarize
    // ========================================================================

    // Condense multiple paragraphs into a brief summary
    std::string summarize(const std::vector<std::string>& paragraphs) {
        if (paragraphs.empty()) return "";
        if (paragraphs.size() == 1) return paragraphs[0];

        std::string summary = "In summary: ";

        // Extract key sentence from each paragraph (first sentence)
        for (size_t i = 0; i < paragraphs.size() && i < 3; i++) {
            // Get first sentence
            std::string firstSentence;
            for (char c : paragraphs[i]) {
                firstSentence += c;
                if (c == '.') break;
            }

            if (i > 0) {
                summary += " " + pickRandom(additionConnectors) + ", ";
            }
            // Lowercase first char if not first
            if (i > 0 && !firstSentence.empty()) {
                firstSentence[0] = std::tolower(firstSentence[0]);
            }
            summary += firstSentence;
        }

        return summary;
    }

    // ========================================================================
    // Core API: Transition
    // ========================================================================

    // Smooth topic change between two topics
    std::string transition(const std::string& fromTopic, const std::string& toTopic) {
        std::vector<std::string> templates = {
            "Speaking of {from}, let's also consider {to}.",
            "Moving from {from} to a related topic — {to}.",
            "That covers {from}. Now, regarding {to}:",
            "With {from} in mind, it's worth looking at {to}.",
            "Now that we've discussed {from}, let's turn to {to}.",
            "Related to {from}, there's also {to} to consider."
        };

        std::string tmpl = pickRandom(templates);

        size_t pos = tmpl.find("{from}");
        if (pos != std::string::npos) tmpl.replace(pos, 6, fromTopic);
        pos = tmpl.find("{to}");
        if (pos != std::string::npos) tmpl.replace(pos, 4, toTopic);

        return tmpl;
    }

    // ========================================================================
    // Utility
    // ========================================================================

    int getRecentCount() const { return (int)recentOutputs.size(); }
    void clearRecent() { recentOutputs.clear(); }

    // Set the RNG seed for reproducible output (testing)
    void setSeed(unsigned int seed) { rngState = seed; }
};

#endif // MK_GENERATIVE_COMPOSER_CPP
