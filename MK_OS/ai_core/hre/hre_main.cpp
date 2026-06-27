#ifndef MK_HRE_MAIN_CPP
#define MK_HRE_MAIN_CPP

// ===================================================================================
// MK HYBRID REASONING ENGINE - MASTER INTEGRATION (THE BRAIN OF MK)
// ===================================================================================
// This is the central loop that ties everything together:
//   1. pattern_graph.cpp  - Knowledge storage and retrieval
//   2. reasoning_chains.cpp - Logical inference and derivation
//   3. query_parser.cpp   - Natural language understanding
//   4. composer.cpp       - Natural language response generation
//
// Flow:
//   User input -> Parse intent -> Route to handler -> Compose response -> Display
//
// Intent routing:
//   QUESTION  -> search graph -> try reasoning -> compose answer
//   STATEMENT -> extract fact -> add to graph -> persist to file -> confirm
//   COMMAND   -> execute (remember, forget, status, help, etc.)
//   GREETING  -> respond warmly
//
// This is an interactive REPL. It loops forever until "quit" or "exit".
// Designed for extremely low-end hardware. Pure C++ STL. No external deps.
// ===================================================================================

#include "pattern_graph.cpp"
#include "reasoning_chains.cpp"
#include "query_parser.cpp"
#include "composer.cpp"

#include <iostream>
#include <string>
#include <algorithm>

class MKBrain {
private:
    MKPatternGraph graph;
    MKReasoningChains reasoner;
    MKQueryParser parser;
    MKComposer composer;

    bool running;
    int interactions;
    std::string knowledge_path;

public:
    MKBrain(const std::string& knowledgeDir = "knowledge_files")
        : graph(knowledgeDir), reasoner(knowledgeDir), 
          composer(MKComposerMode::FRIENDLY),
          running(true), interactions(0), knowledge_path(knowledgeDir) {
        
        std::cout << "\n";
        std::cout << "=========================================\n";
        std::cout << "  MK HYBRID REASONING ENGINE v1.0\n";
        std::cout << "  A new kind of AI. Not an LLM.\n";
        std::cout << "  Graph-based. Instant learning.\n";
        std::cout << "=========================================\n";
        std::cout << "\n";
    }

    // ─────────────────────────────────────────
    //  INITIALIZATION
    // ─────────────────────────────────────────

    void initialize() {
        std::cout << "[MK BRAIN] Loading knowledge base...\n";
        graph.loadAllKnowledge();
        
        std::cout << "[MK BRAIN] Loading custom reasoning rules...\n";
        reasoner.loadRulesFromFile("rules.mk");

        std::cout << "[MK BRAIN] Initialization complete.\n";
        graph.printStats();
        std::cout << "\nType 'help' for commands. Ask me anything or teach me something!\n\n";
    }

    // ─────────────────────────────────────────
    //  MAIN LOOP (REPL)
    // ─────────────────────────────────────────

    void run() {
        initialize();

        while (running) {
            // Display prompt
            std::cout << "MK> ";
            std::cout.flush();

            // Read input
            std::string input;
            if (!std::getline(std::cin, input)) {
                // EOF reached
                running = false;
                break;
            }

            // Skip empty input
            if (input.empty() || input.find_first_not_of(" \t\r\n") == std::string::npos) {
                continue;
            }

            // Process input and generate response
            std::string response = processInput(input);
            interactions++;

            // Display response
            std::cout << "\n" << response << "\n\n";
        }

        shutdown();
    }

    // ─────────────────────────────────────────
    //  INPUT PROCESSING & ROUTING
    // ─────────────────────────────────────────

    std::string processInput(const std::string& input) {
        // Step 1: Parse the input
        MKParsedQuery query = parser.parse(input);

        // Step 2: Route based on intent
        switch (query.intent) {
            case MKIntentType::QUESTION:
                return handleQuestion(query);

            case MKIntentType::STATEMENT:
                return handleStatement(query);

            case MKIntentType::COMMAND:
                return handleCommand(query);

            case MKIntentType::GREETING:
                return handleGreeting(query);

            case MKIntentType::UNKNOWN:
            default:
                return handleUnknown(query);
        }
    }

    // ─────────────────────────────────────────
    //  HANDLER: QUESTIONS
    // ─────────────────────────────────────────

    std::string handleQuestion(const MKParsedQuery& query) {
        MKResponseContext ctx;
        ctx.subject = query.subject;
        ctx.relation = query.relation;
        ctx.object = query.object;
        ctx.is_partial = false;
        ctx.confidence = 0.0f;

        // Strategy 1: Direct graph lookup
        if (!query.subject.empty() && !query.relation.empty()) {
            auto results = graph.query(query.subject, query.relation);
            if (!results.empty()) {
                ctx.facts_found = results;
                ctx.confidence = 1.0f;
                return composer.composeAnswer(ctx);
            }
        }

        // Strategy 2: Check if asking "does X relation Y?" (yes/no question)
        if (!query.subject.empty() && !query.relation.empty() && !query.object.empty()) {
            bool exists = graph.hasFact(query.subject, query.relation, query.object);
            if (exists) {
                ctx.facts_found.push_back(query.object);
                ctx.confidence = 1.0f;
                return "Yes! " + capitalize(query.subject) + " " + 
                       composer.humanizeRelation(query.relation) + " " + query.object + ".";
            }
        }

        // Strategy 3: Try reasoning chains (inference)
        if (!query.subject.empty() && !query.relation.empty()) {
            auto inferences = reasoner.reason(query.subject, query.relation, graph);
            if (!inferences.empty()) {
                for (const auto& inf : inferences) {
                    ctx.inferred_facts.push_back(inf.derived_target);
                }
                // Build reasoning chain explanation
                if (!inferences[0].chain.empty()) {
                    ctx.reasoning_chain = inferences[0].rule_name;
                }
                ctx.confidence = inferences[0].confidence;
                return composer.composeAnswer(ctx);
            }
        }

        // Strategy 4: Path query (multi-hop graph traversal)
        if (!query.subject.empty() && !query.relation.empty()) {
            auto pathResult = graph.pathQuery(query.subject, query.relation, 3);
            if (pathResult.found) {
                ctx.inferred_facts.push_back(pathResult.answer);
                ctx.reasoning_chain = "graph traversal (" + 
                                      std::to_string(pathResult.hops) + " hops)";
                ctx.confidence = pathResult.confidence;
                return composer.composeAnswer(ctx);
            }
        }

        // Strategy 5: Get all facts about the subject (partial answer)
        if (!query.subject.empty()) {
            auto allFacts = graph.getAll(query.subject);
            if (!allFacts.empty()) {
                ctx.is_partial = true;
                for (const auto& edge : allFacts) {
                    ctx.facts_found.push_back(edge.relation + " " + edge.target);
                }
                return composer.composeAnswer(ctx);
            }
        }

        // Strategy 6: Try keywords as subjects
        for (const auto& keyword : query.keywords) {
            auto allFacts = graph.getAll(keyword);
            if (!allFacts.empty()) {
                std::vector<std::pair<std::string, std::string>> factPairs;
                for (const auto& edge : allFacts) {
                    factPairs.push_back({edge.relation, edge.target});
                }
                return composer.composeFactList(keyword, factPairs);
            }
        }

        // No answer found
        return composer.composeAnswer(ctx);
    }

    // ─────────────────────────────────────────
    //  HANDLER: STATEMENTS (LEARNING)
    // ─────────────────────────────────────────

    std::string handleStatement(const MKParsedQuery& query) {
        if (query.subject.empty() || query.object.empty()) {
            return "I couldn't quite parse that fact. Try: 'X is Y' or 'X has Y'.";
        }

        std::string relation = query.relation.empty() ? "is_a" : query.relation;

        // Check if we already know this
        if (graph.hasFact(query.subject, relation, query.object)) {
            return "I already know that " + query.subject + " " + 
                   composer.humanizeRelation(relation) + " " + query.object + "!";
        }

        // Learn the new fact (add to graph + persist to file)
        graph.persistNewFact(query.subject, relation, query.object);

        return composer.composeConfirmation(query.subject, relation, query.object);
    }

    // ─────────────────────────────────────────
    //  HANDLER: COMMANDS
    // ─────────────────────────────────────────

    std::string handleCommand(const MKParsedQuery& query) {
        std::string cmd = query.command_verb;

        // --- HELP ---
        if (cmd == "help") {
            return composer.composeHelp();
        }

        // --- STATUS / STATS ---
        if (cmd == "status" || cmd == "stats") {
            return composer.composeStatus(
                graph.nodeCount(), graph.edgeCount(),
                reasoner.ruleCount(), graph.queryCount()
            );
        }

        // --- QUIT / EXIT ---
        if (cmd == "quit" || cmd == "exit" || cmd == "bye") {
            running = false;
            return "Goodbye! I'll remember everything for next time. See you!";
        }

        // --- TELL ABOUT ---
        if (cmd == "tell_about") {
            if (query.subject.empty()) {
                return "Tell you about what? Give me a subject!";
            }
            auto allFacts = graph.getAll(query.subject);
            if (allFacts.empty()) {
                // Try keywords
                for (const auto& kw : query.keywords) {
                    allFacts = graph.getAll(kw);
                    if (!allFacts.empty()) {
                        std::vector<std::pair<std::string, std::string>> pairs;
                        for (const auto& e : allFacts) pairs.push_back({e.relation, e.target});
                        return composer.composeParagraph(kw, pairs);
                    }
                }
                return "I don't know anything about " + query.subject + " yet. Teach me!";
            }
            std::vector<std::pair<std::string, std::string>> pairs;
            for (const auto& e : allFacts) pairs.push_back({e.relation, e.target});
            return composer.composeParagraph(query.subject, pairs);
        }

        // --- REMEMBER / LEARN / TEACH ---
        if (cmd == "remember" || cmd == "learn" || cmd == "teach") {
            if (query.subject.empty() || query.object.empty()) {
                return "What should I remember? Use format: 'remember [subject] is/has [object]'";
            }
            std::string relation = query.relation.empty() ? "is_a" : query.relation;
            graph.persistNewFact(query.subject, relation, query.object);
            return composer.composeConfirmation(query.subject, relation, query.object);
        }

        // --- FORGET ---
        if (cmd == "forget") {
            // Note: Forgetting is not fully implemented in graph yet
            // For safety, MK remembers everything. This is by design.
            return "I prefer not to forget things -- knowledge is precious! "
                   "But I can learn new facts that override old ones.";
        }

        // --- DUMP ---
        if (cmd == "dump") {
            graph.dump();
            return "Knowledge dumped to console (see above).";
        }

        // --- SAVE ---
        if (cmd == "save") {
            graph.saveToFile("full_dump.mk");
            return "All knowledge saved to full_dump.mk!";
        }

        // --- RULES ---
        if (cmd == "rules") {
            reasoner.listRules();
            return "Reasoning rules listed above.";
        }

        // --- SHOW / LIST ---
        if (cmd == "show" || cmd == "list") {
            if (query.subject.empty()) {
                graph.printStats();
                return "Stats shown above.";
            }
            auto facts = graph.getAll(query.subject);
            if (facts.empty()) return "Nothing found for: " + query.subject;
            std::vector<std::pair<std::string, std::string>> pairs;
            for (const auto& e : facts) pairs.push_back({e.relation, e.target});
            return composer.composeFactList(query.subject, pairs);
        }

        // --- EXPLAIN ---
        if (cmd == "explain") {
            return "I'm MK, a Hybrid Reasoning Engine. I work by:\n"
                   "1. Storing knowledge as a graph (nodes = concepts, edges = relations)\n"
                   "2. Parsing your input to understand intent\n"
                   "3. Searching my graph + applying logic rules to find answers\n"
                   "4. Composing natural responses from templates\n"
                   "I'm NOT an LLM. I don't hallucinate. I either know something or I don't.";
        }

        // Unknown command
        return "Unknown command: '" + cmd + "'. Type 'help' for available commands.";
    }

    // ─────────────────────────────────────────
    //  HANDLER: GREETINGS
    // ─────────────────────────────────────────

    std::string handleGreeting(const MKParsedQuery& query) {
        return composer.composeGreeting();
    }

    // ─────────────────────────────────────────
    //  HANDLER: UNKNOWN
    // ─────────────────────────────────────────

    std::string handleUnknown(const MKParsedQuery& query) {
        // Try treating it as a question about the first keyword
        if (!query.keywords.empty()) {
            auto facts = graph.getAll(query.keywords[0]);
            if (!facts.empty()) {
                std::vector<std::pair<std::string, std::string>> pairs;
                for (const auto& e : facts) pairs.push_back({e.relation, e.target});
                return composer.composeFactList(query.keywords[0], pairs);
            }
        }
        return composer.composeError();
    }

    // ─────────────────────────────────────────
    //  SHUTDOWN
    // ─────────────────────────────────────────

    void shutdown() {
        std::cout << "\n[MK BRAIN] Shutting down...\n";
        std::cout << "[MK BRAIN] Session stats: " << interactions << " interactions.\n";
        graph.printStats();
        reasoner.printStats();
        parser.printStats();
        composer.printStats();
        std::cout << "[MK BRAIN] Goodbye.\n";
    }

    // ─────────────────────────────────────────
    //  UTILITY
    // ─────────────────────────────────────────

    bool isRunning() const { return running; }
    int getInteractions() const { return interactions; }

private:
    std::string capitalize(const std::string& s) {
        if (s.empty()) return s;
        std::string result = s;
        result[0] = std::toupper(result[0]);
        return result;
    }
};

// ===================================================================================
//  Module-only file: provides MKBrain class for the Hybrid Reasoning Engine.
//  No standalone entry point - mk_entry.cpp handles the REPL loop.
// ===================================================================================

#endif // MK_HRE_MAIN_CPP
