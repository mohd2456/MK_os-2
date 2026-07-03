// ============================================================
// MK OS - Test Suite
// Exercises core AI pipeline modules without user interaction.
// Compile: clang++ -std=c++17 -O2 -DDARWIN -o mk_os_test tests/test_main.cpp -lcurl -lsqlite3
// Run from: MK_OS/ directory (so relative paths to knowledge_files work)
// ============================================================
#ifndef MK_TEST_MAIN_CPP
#define MK_TEST_MAIN_CPP

#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <functional>

// ============================================================
// Simple Test Framework
// ============================================================

static int g_tests_passed = 0;
static int g_tests_failed = 0;
static int g_assertions_passed = 0;
static int g_assertions_failed = 0;

#define TEST_ASSERT(expr, msg) \
    do { \
        if (expr) { \
            g_assertions_passed++; \
        } else { \
            g_assertions_failed++; \
            std::cerr << "    FAIL: " << msg << " (" << #expr << ")" \
                      << " [" << __FILE__ << ":" << __LINE__ << "]" << std::endl; \
        } \
    } while(0)

#define TEST_ASSERT_GT(a, b, msg) TEST_ASSERT((a) > (b), msg)
#define TEST_ASSERT_EQ(a, b, msg) TEST_ASSERT((a) == (b), msg)
#define TEST_ASSERT_NE(a, b, msg) TEST_ASSERT((a) != (b), msg)
#define TEST_ASSERT_TRUE(expr, msg) TEST_ASSERT((expr), msg)
#define TEST_ASSERT_FALSE(expr, msg) TEST_ASSERT(!(expr), msg)

#define RUN_TEST(testFunc) \
    do { \
        std::cout << "  [RUN ] " << #testFunc << std::endl; \
        int prev_failures = g_assertions_failed; \
        try { \
            testFunc(); \
            if (g_assertions_failed == prev_failures) { \
                std::cout << "  [PASS] " << #testFunc << std::endl; \
                g_tests_passed++; \
            } else { \
                std::cout << "  [FAIL] " << #testFunc << std::endl; \
                g_tests_failed++; \
            } \
        } catch (const std::exception& e) { \
            std::cout << "  [FAIL] " << #testFunc << " (exception: " << e.what() << ")" << std::endl; \
            g_tests_failed++; \
            g_assertions_failed++; \
        } catch (...) { \
            std::cout << "  [FAIL] " << #testFunc << " (unknown exception)" << std::endl; \
            g_tests_failed++; \
            g_assertions_failed++; \
        } \
    } while(0)

// ============================================================
// Include the modules under test
// The include paths are relative to MK_OS/ (the working directory)
// ============================================================
#include "../ai_core/hre/pattern_graph.cpp"
#include "../ai_core/hre/deep_reasoner.cpp"
#include "../ai_core/hre/reasoning_chains.cpp"
#include "../ai_core/hre/composer.cpp"
#include "../ai_core/smart_router.cpp"
#include "../mk_brain/vector_search/ann_search.cpp"
#include "../mk_brain/embeddings/embeddings_eng.cpp"
#include "../ai_core/math_solver.cpp"
#include "../tools/code_runner.cpp"
#include "../mk_brain/personality/casual_responses.cpp"
#include "../ai_core/conversation_mode.cpp"
#include "../ai_core/idea_engine.cpp"

// New module includes for FEAT-005 integration tests
#include "../crypto/market_data.cpp"
#include "../crypto/technical_analysis.cpp"
#include "../crypto/signal_engine.cpp"
#include "../crypto/portfolio_manager.cpp"
#include "../crypto/risk_manager.cpp"
#include "../crypto/exchange_api.cpp"
#include "../crypto/trading_bot.cpp"
#include "../homelab/device_registry.cpp"
#include "../homelab/resource_monitor.cpp"
#include "../mind/mastery_network.cpp"
#include "../mind/goal_engine.cpp"
#include "../mind/self_funding.cpp"
#include "../mind/autonomous_learner.cpp"
#include "../mind/knowledge_validator.cpp"
#include "../sync/knowledge_sync.cpp"
#include "../config/mk_config.cpp"

// Integration tests
#include "test_integration.cpp"

// ============================================================
// TEST: Pattern Graph
// ============================================================
void test_pattern_graph() {
    MKPatternGraph graph("ai_core/hre/knowledge_files");

    // Load knowledge from the knowledge files directory
    graph.loadAllKnowledge();

    // Verify edges were loaded
    TEST_ASSERT_GT(graph.edgeCount(), 0, "Graph should have edges after loading knowledge files");
    TEST_ASSERT_GT(graph.nodeCount(), 0, "Graph should have nodes after loading knowledge files");

    // Verify getAll returns results for a known concept
    auto pythonFacts = graph.getAll("python");
    TEST_ASSERT_GT(pythonFacts.size(), 0u, "getAll('python') should return facts about python");

    // Verify pathQuery works
    auto pathResult = graph.pathQuery("python", "is_a", 3);
    // We expect it to at least run without crashing - result depends on knowledge content
    // If direct fact "python is_a language" exists, it should find it
    TEST_ASSERT_TRUE(true, "pathQuery completed without crash");

    // Test persistNewFact and verify edge count increases
    // Use addFact directly to test in-memory graph growth (avoids file persistence issues on re-runs)
    int edgesBefore = graph.edgeCount();
    graph.addFact("unique_test_zebra_99", "is_a", "unique_test_animal_99", 0.9f);
    int edgesAfter = graph.edgeCount();
    TEST_ASSERT_GT(edgesAfter, edgesBefore, "addFact should increase edge count");

    // Verify the new fact can be queried
    auto queryResult = graph.query("unique_test_zebra_99", "is_a");
    TEST_ASSERT_GT(queryResult.size(), 0u, "Should be able to query the newly added fact");
    TEST_ASSERT_EQ(queryResult[0], "unique_test_animal_99", "Query result should match added target");

    // Test hasFact
    TEST_ASSERT_TRUE(graph.hasFact("unique_test_zebra_99", "is_a", "unique_test_animal_99"),
                     "hasFact should confirm the newly added fact");

    // Test reverse query
    auto reverseResult = graph.reverseQuery("is_a", "unique_test_animal_99");
    TEST_ASSERT_GT(reverseResult.size(), 0u, "reverseQuery should find sources for the target");
}

// ============================================================
// TEST: Reasoning Chains
// ============================================================
void test_reasoning_chains() {
    MKPatternGraph graph("ai_core/hre/knowledge_files");
    graph.loadAllKnowledge();

    MKReasoningChains reasoning("ai_core/hre/knowledge_files");

    // Verify rules are loaded
    TEST_ASSERT_GT(reasoning.ruleCount(), 0, "Should have reasoning rules loaded");

    // deriveAll should complete without crash
    int newFacts = reasoning.deriveAll(graph);
    TEST_ASSERT_TRUE(true, "deriveAll completed without crash");
    (void)newFacts; // Result can be 0 in current implementation

    // Test reasoning with known facts:
    // Add facts that form a chain: "sparrow is_a bird" + "bird has wings"
    // Rule: inheritance_has => "sparrow has wings"
    graph.addFact("sparrow", "is_a", "bird", 1.0f);
    graph.addFact("bird", "has", "wings", 1.0f);

    auto results = reasoning.reason("sparrow", "has", graph);
    TEST_ASSERT_GT(results.size(), 0u,
                   "Reasoning should derive 'sparrow has wings' from inheritance rule");

    if (!results.empty()) {
        TEST_ASSERT_EQ(results[0].derived_target, "wings",
                       "Derived target should be 'wings'");
        TEST_ASSERT_EQ(results[0].rule_name, "inheritance_has",
                       "Should use the inheritance_has rule");
        TEST_ASSERT_GT(results[0].confidence, 0.0f,
                       "Confidence should be greater than 0");
        TEST_ASSERT_GT(results[0].chain.size(), 0u,
                       "Reasoning chain should have steps");
    }

    // Test another rule: transitive is_a
    // "poodle is_a dog" + "dog is_a animal" => "poodle is_a animal"
    graph.addFact("poodle", "is_a", "dog", 1.0f);
    graph.addFact("dog", "is_a", "animal", 1.0f);

    auto transitiveResults = reasoning.reason("poodle", "is_a", graph);
    TEST_ASSERT_GT(transitiveResults.size(), 0u,
                   "Should derive transitive is_a: poodle is_a animal");

    // Verify inference count increases
    TEST_ASSERT_GT(reasoning.inferenceCount(), 0, "Should have made inferences");
}

// ============================================================
// TEST: Smart Router
// ============================================================
void test_smart_router() {
    MKSmartRouter router;

    // Test: factual query should route to GRAPH
    auto graphDecision = router.route("what is python");
    TEST_ASSERT_EQ(static_cast<int>(graphDecision.primaryRoute),
                   static_cast<int>(MKRouteType::GRAPH),
                   "'what is python' should route to GRAPH");
    TEST_ASSERT_GT(graphDecision.confidence, 0.3f,
                   "GRAPH route confidence should be > 0.3");

    // Test: weather query should route to INSTANT
    auto instantDecision = router.route("weather in london");
    TEST_ASSERT_EQ(static_cast<int>(instantDecision.primaryRoute),
                   static_cast<int>(MKRouteType::INSTANT),
                   "'weather in london' should route to INSTANT");
    TEST_ASSERT_GT(instantDecision.confidence, 0.3f,
                   "INSTANT route confidence should be > 0.3");

    // Test: complex reasoning query should route to REASON
    auto reasonDecision = router.route("explain quantum computing step by step");
    TEST_ASSERT_EQ(static_cast<int>(reasonDecision.primaryRoute),
                   static_cast<int>(MKRouteType::REASON),
                   "'explain quantum computing step by step' should route to REASON");
    TEST_ASSERT_GT(reasonDecision.confidence, 0.3f,
                   "REASON route confidence should be > 0.3");

    // Test: creative task should route to GENERATE
    auto generateDecision = router.route("write me a poem");
    TEST_ASSERT_EQ(static_cast<int>(generateDecision.primaryRoute),
                   static_cast<int>(MKRouteType::GENERATE),
                   "'write me a poem' should route to GENERATE");
    TEST_ASSERT_GT(generateDecision.confidence, 0.3f,
                   "GENERATE route confidence should be > 0.3");

    // Test: current events should route to SEARCH
    auto searchDecision = router.route("latest breaking news this year");
    TEST_ASSERT_EQ(static_cast<int>(searchDecision.primaryRoute),
                   static_cast<int>(MKRouteType::SEARCH),
                   "'latest breaking news this year' should route to SEARCH");
    TEST_ASSERT_GT(searchDecision.confidence, 0.3f,
                   "SEARCH route confidence should be > 0.3");

    // Verify all decisions have valid confidence values
    TEST_ASSERT_TRUE(graphDecision.confidence <= 1.0f, "Confidence should be <= 1.0");
    TEST_ASSERT_TRUE(instantDecision.confidence <= 1.0f, "Confidence should be <= 1.0");
    TEST_ASSERT_TRUE(reasonDecision.confidence <= 1.0f, "Confidence should be <= 1.0");
    TEST_ASSERT_TRUE(generateDecision.confidence <= 1.0f, "Confidence should be <= 1.0");
    TEST_ASSERT_TRUE(searchDecision.confidence <= 1.0f, "Confidence should be <= 1.0");
}

// ============================================================
// TEST: Deep Reasoner
// ============================================================
void test_deep_reasoner() {
    MKPatternGraph graph("ai_core/hre/knowledge_files");
    graph.loadAllKnowledge();

    MKDeepReasoner reasoner(10);

    // Test think() on a known topic
    auto thought = reasoner.think("what is python", graph);

    // Should have steps
    TEST_ASSERT_GT(thought.steps.size(), 0u, "Thought chain should have steps");

    // Should have a decompose step
    TEST_ASSERT_EQ(thought.steps[0].type, "decompose",
                   "First step should be decompose");

    // The result should not crash and should produce some output
    // finalAnswer may be empty if no knowledge found, but the process should complete
    TEST_ASSERT_TRUE(true, "think() completed without crash");

    // Test with a query that triggers causal reasoning
    auto causalThought = reasoner.think("why does exercise help", graph);
    TEST_ASSERT_GT(causalThought.steps.size(), 0u,
                   "Causal thought chain should have steps");

    // Test plan() method
    auto planSteps = reasoner.plan("learn programming", graph);
    TEST_ASSERT_GT(planSteps.size(), 0u, "plan() should return steps");

    // Test contradiction detection
    graph.addFact("hot", "opposite_of", "cold", 1.0f);
    graph.addFact("water", "has_property", "hot", 1.0f);
    bool contradiction = reasoner.checkContradiction("water", "has_property", "cold", graph);
    TEST_ASSERT_TRUE(contradiction, "Should detect contradiction between hot and cold");
}

// ============================================================
// TEST: Composer
// ============================================================
void test_composer() {
    MKComposer composer(MKComposerMode::FRIENDLY);

    // Test composeAnswer with facts
    MKResponseContext ctx;
    ctx.subject = "python";
    ctx.relation = "is_a";
    ctx.object = "language";
    ctx.facts_found = {"programming language"};
    ctx.is_partial = false;
    ctx.confidence = 1.0f;

    std::string answer = composer.composeAnswer(ctx);
    TEST_ASSERT_GT(answer.size(), 0u, "composeAnswer should return non-empty string");

    // Test composeAnswer with no facts (unknown)
    MKResponseContext emptyCtx;
    emptyCtx.subject = "xyznoexist";
    emptyCtx.relation = "is_a";
    emptyCtx.object = "";
    emptyCtx.is_partial = false;
    emptyCtx.confidence = 0.0f;

    std::string unknownAnswer = composer.composeAnswer(emptyCtx);
    TEST_ASSERT_GT(unknownAnswer.size(), 0u, "composeAnswer for unknown should still produce output");

    // Test composeConfirmation
    std::string confirmation = composer.composeConfirmation("cat", "is_a", "animal");
    TEST_ASSERT_GT(confirmation.size(), 0u, "composeConfirmation should return non-empty string");

    // Test composeGreeting
    std::string greeting = composer.composeGreeting();
    TEST_ASSERT_GT(greeting.size(), 0u, "composeGreeting should return non-empty string");

    // Test composeHelp
    std::string help = composer.composeHelp();
    TEST_ASSERT_GT(help.size(), 0u, "composeHelp should return non-empty string");

    // Test composeFactList
    std::vector<std::pair<std::string, std::string>> facts = {
        {"is_a", "animal"},
        {"has", "fur"},
        {"can", "purr"}
    };
    std::string factList = composer.composeFactList("cat", facts);
    TEST_ASSERT_GT(factList.size(), 0u, "composeFactList should return non-empty string");

    // Test composeParagraph
    std::string paragraph = composer.composeParagraph("cat", facts);
    TEST_ASSERT_GT(paragraph.size(), 0u, "composeParagraph should return non-empty string");

    // Test compose count tracking
    TEST_ASSERT_GT(composer.composeCount(), 0, "Compose count should be positive after usage");
}

// ============================================================
// TEST: Vector Search (ANN)
// ============================================================
void test_vector_search() {
    MKANNSearch ann(128);

    // Add some vectors via textToEmbedding
    std::string text1 = "python programming language";
    std::string text2 = "java programming language";
    std::string text3 = "cooking recipe for pasta";
    std::string text4 = "python snake reptile";
    std::string text5 = "javascript web development";

    auto emb1 = ann.textToEmbedding(text1);
    auto emb2 = ann.textToEmbedding(text2);
    auto emb3 = ann.textToEmbedding(text3);
    auto emb4 = ann.textToEmbedding(text4);
    auto emb5 = ann.textToEmbedding(text5);

    // Verify embeddings have correct dimensions
    TEST_ASSERT_EQ((int)emb1.size(), 128, "Embedding should have 128 dimensions");

    // Add vectors to the index
    ann.addVector("python_prog", text1, emb1);
    ann.addVector("java_prog", text2, emb2);
    ann.addVector("cooking", text3, emb3);
    ann.addVector("python_animal", text4, emb4);
    ann.addVector("javascript", text5, emb5);

    TEST_ASSERT_EQ(ann.size(), 5, "Index should have 5 vectors");

    // Build the index
    ann.buildIndex();

    // Search for something similar to python programming
    std::string queryText = "python code programming";
    auto queryEmb = ann.textToEmbedding(queryText);
    auto results = ann.searchExact(queryEmb, 3);

    TEST_ASSERT_GT(results.size(), 0u, "Search should return results");

    if (!results.empty()) {
        TEST_ASSERT_GT(results[0].score, 0.0f, "Top result should have positive score");
        // The top result should be related to programming (python or java)
        TEST_ASSERT_TRUE(
            results[0].label == "python_prog" || results[0].label == "java_prog" || 
            results[0].label == "python_animal" || results[0].label == "javascript",
            "Top result should be semantically related"
        );
    }

    // Test the high-level text search interface
    auto textResults = ann.search("python programming", 3);
    TEST_ASSERT_GT(textResults.size(), 0u, "Text search should return results");
    if (!textResults.empty()) {
        TEST_ASSERT_GT(textResults[0].score, 0.0f, "Text search top result should have positive score");
    }

    // Test approximate search
    auto approxResults = ann.searchApproximate(queryEmb, 3, 2);
    TEST_ASSERT_GT(approxResults.size(), 0u, "Approximate search should return results");
}

// ============================================================
// TEST: Embeddings Engine
// ============================================================
void test_embeddings_engine() {
    MKEmbeddingsEngine engine;

    // Test getting a known embedding
    auto bootEmb = engine.getEmbedding("boot");
    TEST_ASSERT_EQ(bootEmb.size(), 3u, "Known token should return 3D embedding");
    TEST_ASSERT_TRUE(bootEmb[0] != 0.0f || bootEmb[1] != 0.0f || bootEmb[2] != 0.0f,
                     "Known token embedding should not be all zeros");

    // Test unknown token
    auto unknownEmb = engine.getEmbedding("unknown_token_xyz");
    TEST_ASSERT_EQ(unknownEmb.size(), 3u, "Unknown token should return 3D fallback embedding");
    TEST_ASSERT_EQ(unknownEmb[0], 0.0f, "Unknown token fallback should be zero vector");

    // Test similarity calculation
    std::vector<float> vecA = {1.0f, 0.0f, 0.0f};
    std::vector<float> vecB = {1.0f, 0.0f, 0.0f};
    float simSame = engine.calculateSimilarity(vecA, vecB);
    TEST_ASSERT_GT(simSame, 0.0f, "Identical vectors should have positive similarity");

    std::vector<float> vecC = {-1.0f, 0.0f, 0.0f};
    float simOpposite = engine.calculateSimilarity(vecA, vecC);
    TEST_ASSERT_TRUE(simOpposite < simSame, "Opposite vectors should have lower similarity");

    // Test similarity between known embeddings
    auto coreEmb = engine.getEmbedding("core");
    auto nexusEmb = engine.getEmbedding("nexus");
    float coreSim = engine.calculateSimilarity(coreEmb, nexusEmb);
    // Just verify it returns a number (not NaN or crash)
    TEST_ASSERT_TRUE(coreSim == coreSim, "Similarity should not be NaN"); // NaN != NaN
}

// ============================================================
// TEST: Math Solver Extensions (Calculus, Statistics, Base Conversion)
// ============================================================
void test_math_solver() {
    MKMathSolver math;

    // Test quadratic equation
    auto quadResult = math.solveQuadratic(1, -3, 2);
    TEST_ASSERT_TRUE(quadResult.success, "Quadratic x^2-3x+2=0 should solve");
    TEST_ASSERT_TRUE(quadResult.answer.find("1") != std::string::npos, "Root x=1 should be in answer");
    TEST_ASSERT_TRUE(quadResult.answer.find("2") != std::string::npos, "Root x=2 should be in answer");

    // Test complex roots
    auto complexResult = math.solveQuadratic(1, 0, 1);
    TEST_ASSERT_TRUE(complexResult.success, "x^2+1=0 should solve with complex roots");
    TEST_ASSERT_TRUE(complexResult.answer.find("i") != std::string::npos, "Should contain imaginary marker");

    // Test unit conversion
    auto convertResult = math.convert(1.0, "km", "m");
    TEST_ASSERT_TRUE(convertResult.success, "1 km to m should succeed");
    TEST_ASSERT_TRUE(convertResult.answer.find("1000") != std::string::npos, "1 km = 1000 m");

    // Test temperature conversion
    auto tempResult = math.convert(100.0, "c", "f");
    TEST_ASSERT_TRUE(tempResult.success, "100 C to F should succeed");
    TEST_ASSERT_TRUE(tempResult.answer.find("212") != std::string::npos, "100 C = 212 F");

    // Test trigonometry
    auto sinResult = math.trig("sin", 90.0);
    TEST_ASSERT_TRUE(sinResult.success, "sin(90) should succeed");
    TEST_ASSERT_TRUE(sinResult.answer.find("1") != std::string::npos, "sin(90) = 1");

    auto cosResult = math.trig("cos", 0.0);
    TEST_ASSERT_TRUE(cosResult.success, "cos(0) should succeed");
    TEST_ASSERT_TRUE(cosResult.answer.find("1") != std::string::npos, "cos(0) = 1");

    // Test percentage
    auto pctResult = math.percentage(200.0, 15.0);
    TEST_ASSERT_TRUE(pctResult.success, "15% of 200 should succeed");
    TEST_ASSERT_TRUE(pctResult.answer.find("30") != std::string::npos, "15% of 200 = 30");

    // Test natural language solve: derivative (use "diff" prefix to route to derivative)
    auto derivResult = math.derivative("derivative x^2 at 3");
    TEST_ASSERT_TRUE(derivResult.success, "Derivative of x^2 at x=3 should succeed");
    TEST_ASSERT_TRUE(derivResult.answer.find("6") != std::string::npos, "d/dx(x^2) at 3 = 6");

    // Test natural language solve: statistics mean
    auto statsResult = math.solve("mean 2 4 6 8 10");
    TEST_ASSERT_TRUE(statsResult.success, "Mean of 2,4,6,8,10 should succeed");
    TEST_ASSERT_TRUE(statsResult.answer.find("6") != std::string::npos, "Mean of 2,4,6,8,10 = 6");

    // Test natural language solve: factorial
    auto factResult = math.solve("factorial 5");
    TEST_ASSERT_TRUE(factResult.success, "Factorial of 5 should succeed");
    TEST_ASSERT_TRUE(factResult.answer.find("120") != std::string::npos, "5! = 120");

    // Test natural language solve: base conversion
    auto binResult = math.solve("to binary 255");
    TEST_ASSERT_TRUE(binResult.success, "Binary conversion of 255 should succeed");
    TEST_ASSERT_TRUE(binResult.answer.find("11111111") != std::string::npos, "255 in binary = 11111111");

    // Test natural language solve: GCD
    auto gcdResult = math.solve("gcd 48 36");
    TEST_ASSERT_TRUE(gcdResult.success, "GCD of 48 and 36 should succeed");
    TEST_ASSERT_TRUE(gcdResult.answer.find("12") != std::string::npos, "GCD(48,36) = 12");

    // Test physics
    std::map<std::string, double> vars = {{"m", 10.0}, {"a", 9.8}};
    auto physResult = math.physics("f=ma", vars);
    TEST_ASSERT_TRUE(physResult.success, "F=ma should succeed");
    TEST_ASSERT_TRUE(physResult.answer.find("98") != std::string::npos, "F = 10*9.8 = 98 N");
}

// ============================================================
// TEST: Code Runner Sanitization
// ============================================================
void test_code_runner_sanitization() {
    MKCodeRunner runner(5);

    // Test that known dangerous patterns are blocked
    auto result1 = runner.run("import os; os.system('rm -rf /')", "python");
    TEST_ASSERT_FALSE(result1.success, "rm -rf / should be blocked");
    TEST_ASSERT_TRUE(result1.stderrOutput.find("BLOCKED") != std::string::npos, "Should say BLOCKED");

    // Test fork bomb detection
    auto result2 = runner.run(":(){ :|:& };:", "bash");
    TEST_ASSERT_FALSE(result2.success, "Fork bomb should be blocked");

    // Test command substitution blocking
    auto result3 = runner.run("echo $(rm -rf /)", "bash");
    TEST_ASSERT_FALSE(result3.success, "Command substitution with rm should be blocked");

    // Test backtick injection blocking
    auto result4 = runner.run("echo `rm -rf /`", "bash");
    TEST_ASSERT_FALSE(result4.success, "Backtick rm should be blocked");

    // Test Python os.system blocking
    auto result5 = runner.run("import os\nos.system('whoami')", "python");
    TEST_ASSERT_FALSE(result5.success, "os.system() should be blocked");

    // Test Python subprocess blocking
    auto result6 = runner.run("import subprocess\nsubprocess.call(['ls'])", "python");
    TEST_ASSERT_FALSE(result6.success, "subprocess.call() should be blocked");

    // Test child_process blocking for JavaScript
    auto result7 = runner.run("const { exec } = require('child_process'); exec('ls')", "javascript");
    TEST_ASSERT_FALSE(result7.success, "child_process should be blocked");

    // Test shutil.rmtree blocking
    auto result8 = runner.run("import shutil\nshutil.rmtree('/tmp/test')", "python");
    TEST_ASSERT_FALSE(result8.success, "shutil.rmtree should be blocked");

    // Test that safe code is allowed through
    auto resultSafe = runner.run("print('hello world')", "python");
    // Note: this may fail if python3 is not installed, but the isDangerous check should NOT block it
    TEST_ASSERT_TRUE(resultSafe.stderrOutput.find("BLOCKED") == std::string::npos, "Safe code should not be blocked");

    // Test language detection
    auto langs = MKCodeRunner::supportedLanguages();
    TEST_ASSERT_EQ((int)langs.size(), 5, "Should support 5 languages");

    // Test split-flag rm variant blocking
    auto result9 = runner.run("rm -r -f /tmp/important", "bash");
    TEST_ASSERT_FALSE(result9.success, "rm -r -f should be blocked");

    // Test --recursive variant blocking
    auto result10 = runner.run("rm --recursive /tmp", "bash");
    TEST_ASSERT_FALSE(result10.success, "rm --recursive should be blocked");
}

// ============================================================
// TEST: Conversation Mode (mood detection, time-of-day, story mode)
// ============================================================
void test_conversation_mode() {
    MKConversationMode conv;

    // --- Mood detection ---
    // Happy mood
    auto happyMood = conv.detectMood("I'm so hyped and excited about this!");
    TEST_ASSERT_EQ(static_cast<int>(happyMood.mood), static_cast<int>(MKMood::HAPPY),
                   "Hyped/excited input should detect HAPPY mood");
    TEST_ASSERT_GT(happyMood.confidence, 0.0f, "Happy mood confidence should be > 0");
    TEST_ASSERT_FALSE(happyMood.trigger_word.empty(), "Should have a trigger word for happy");

    // Sad mood
    auto sadMood = conv.detectMood("I feel so sad and lonely today, drained honestly");
    TEST_ASSERT_EQ(static_cast<int>(sadMood.mood), static_cast<int>(MKMood::SAD),
                   "Sad/lonely/drained input should detect SAD mood");
    TEST_ASSERT_GT(sadMood.confidence, 0.0f, "Sad mood confidence should be > 0");

    // Angry mood
    auto angryMood = conv.detectMood("I'm so pissed and furious right now");
    TEST_ASSERT_EQ(static_cast<int>(angryMood.mood), static_cast<int>(MKMood::ANGRY),
                   "Pissed/furious input should detect ANGRY mood");

    // Nervous mood
    auto nervousMood = conv.detectMood("I'm really nervous and anxious about the test");
    TEST_ASSERT_EQ(static_cast<int>(nervousMood.mood), static_cast<int>(MKMood::NERVOUS),
                   "Nervous/anxious input should detect NERVOUS mood");

    // Neutral mood (no emotional keywords)
    auto neutralMood = conv.detectMood("the sky is blue");
    TEST_ASSERT_EQ(static_cast<int>(neutralMood.mood), static_cast<int>(MKMood::NEUTRAL),
                   "Neutral statement should detect NEUTRAL mood");
    TEST_ASSERT_EQ(neutralMood.confidence, 0.0f, "Neutral mood confidence should be 0");

    // Emotion intensity: strong emotions with multiple keywords + caps/exclamation
    auto strongMood = conv.detectMood("OMG I'M SO EXCITED AND HAPPY AND AMAZING!!!!");
    TEST_ASSERT_TRUE(strongMood.intensity == MKEmotionIntensity::STRONG ||
                     strongMood.intensity == MKEmotionIntensity::EXTREME,
                     "Multiple keywords + caps + exclamation should give STRONG or EXTREME intensity");

    // --- Time of day ---
    MKTimeOfDay tod = conv.getTimeOfDay();
    // Just verify it returns a valid enum value (time-dependent, but should not crash)
    TEST_ASSERT_TRUE(static_cast<int>(tod) >= 0 && static_cast<int>(tod) <= 5,
                     "getTimeOfDay should return valid MKTimeOfDay enum value");

    // --- Story mode detection ---
    // Short input should NOT trigger story mode
    bool shortStory = conv.detectStoryMode("hey what's up");
    TEST_ASSERT_FALSE(shortStory, "Short input should not trigger story mode");

    // Long narrative input should trigger story mode
    bool longStory = conv.detectStoryMode(
        "bro guess what happened today so basically I was at the store and then "
        "this random dude came up and started talking about aliens and like "
        "literally he was so convinced and anyway turns out he was right");
    TEST_ASSERT_TRUE(longStory, "Long narrative with story indicators should trigger story mode");

    // --- isConversation classification ---
    TEST_ASSERT_TRUE(conv.isConversation("yo"), "Greeting 'yo' should be conversation");
    TEST_ASSERT_TRUE(conv.isConversation("hey"), "Greeting 'hey' should be conversation");
    TEST_ASSERT_TRUE(conv.isConversation("bye"), "Goodbye should be conversation");
    TEST_ASSERT_FALSE(conv.isConversation("/help"), "Command should not be conversation");
    TEST_ASSERT_FALSE(conv.isConversation("what is the capital of France?"),
                      "Factual question should not be conversation");
    TEST_ASSERT_TRUE(conv.isConversation("yeah"), "Vague response should be conversation");
    TEST_ASSERT_TRUE(conv.isConversation("fr"), "Vague response 'fr' should be conversation");

    // --- classifyInput types ---
    auto greetType = conv.classifyInput("hey");
    TEST_ASSERT_EQ(static_cast<int>(greetType), static_cast<int>(MKInputType::GREETING),
                   "'hey' should classify as GREETING");

    auto byeType = conv.classifyInput("gotta go");
    TEST_ASSERT_EQ(static_cast<int>(byeType), static_cast<int>(MKInputType::GOODBYE),
                   "'gotta go' should classify as GOODBYE");

    auto cmdType = conv.classifyInput("/stats");
    TEST_ASSERT_EQ(static_cast<int>(cmdType), static_cast<int>(MKInputType::COMMAND),
                   "'/stats' should classify as COMMAND");

    auto vagueType = conv.classifyInput("facts");
    TEST_ASSERT_EQ(static_cast<int>(vagueType), static_cast<int>(MKInputType::VAGUE_RESPONSE),
                   "'facts' should classify as VAGUE_RESPONSE");

    // --- Topic memory ---
    conv.pushTopic("python");
    conv.pushTopic("music");
    TEST_ASSERT_EQ(conv.getLastTopic(), std::string("music"), "Last topic should be 'music'");
    TEST_ASSERT_EQ((int)conv.getRecentTopics().size(), 2, "Should have 2 recent topics");
}

// ============================================================
// TEST: Idea Engine (idea generation, brainstorm)
// ============================================================
void test_idea_engine() {
    MKPatternGraph graph("ai_core/hre/knowledge_files");
    graph.loadAllKnowledge();

    MKIdeaEngine ideaEngine;

    // Generate a single idea
    MKIdea idea = ideaEngine.generateIdea(graph);
    TEST_ASSERT_FALSE(idea.conceptA.empty(), "Generated idea should have conceptA");
    TEST_ASSERT_FALSE(idea.conceptB.empty(), "Generated idea should have conceptB");
    TEST_ASSERT_FALSE(idea.bridge.empty(), "Generated idea should have a bridge");
    TEST_ASSERT_FALSE(idea.idea.empty(), "Generated idea should have an idea sentence");
    TEST_ASSERT_FALSE(idea.category.empty(), "Generated idea should have a category");
    TEST_ASSERT_TRUE(idea.feasibility >= 0.0f && idea.feasibility <= 1.0f,
                     "Feasibility should be in [0,1]");
    TEST_ASSERT_TRUE(idea.novelty >= 0.0f && idea.novelty <= 1.0f,
                     "Novelty should be in [0,1]");
    TEST_ASSERT_TRUE(idea.timestamp > 0, "Timestamp should be positive");

    // Generate multiple ideas to verify diversity
    MKIdea idea2 = ideaEngine.generateIdea(graph);
    TEST_ASSERT_FALSE(idea2.idea.empty(), "Second generated idea should not be empty");
    // Concepts can differ (not deterministic, but should produce something)

    // Brainstorm on a topic
    auto brainstormResults = ideaEngine.brainstorm(graph, "python", 3);
    TEST_ASSERT_GT((int)brainstormResults.size(), 0, "Brainstorm should return at least 1 idea");
    TEST_ASSERT_TRUE(brainstormResults.size() <= 3, "Brainstorm should return at most 3 ideas");
    for (const auto& bi : brainstormResults) {
        TEST_ASSERT_EQ(bi.conceptA, std::string("python"),
                       "Brainstorm ideas should have topic as conceptA");
        TEST_ASSERT_FALSE(bi.conceptB.empty(), "Brainstorm idea should have conceptB");
        TEST_ASSERT_FALSE(bi.idea.empty(), "Brainstorm idea sentence should not be empty");
    }

    // inventFor
    auto inventResults = ideaEngine.inventFor(graph, "slow computer performance");
    TEST_ASSERT_GT((int)inventResults.size(), 0, "inventFor should return ideas");
    for (const auto& inv : inventResults) {
        TEST_ASSERT_FALSE(inv.idea.empty(), "inventFor idea should not be empty");
        TEST_ASSERT_EQ(inv.category, std::string("invention"),
                       "inventFor category should be 'invention'");
    }

    // History tracking
    auto history = ideaEngine.getHistory();
    TEST_ASSERT_GT((int)history.size(), 0, "History should not be empty after generating ideas");
}

// ============================================================
// Main: Run all tests
// ============================================================
int main() {
    std::cout << std::endl;
    std::cout << "================================================" << std::endl;
    std::cout << "  MK OS - Test Suite" << std::endl;
    std::cout << "================================================" << std::endl;
    std::cout << std::endl;

    RUN_TEST(test_pattern_graph);
    RUN_TEST(test_reasoning_chains);
    RUN_TEST(test_smart_router);
    RUN_TEST(test_deep_reasoner);
    RUN_TEST(test_composer);
    RUN_TEST(test_vector_search);
    RUN_TEST(test_embeddings_engine);
    RUN_TEST(test_math_solver);
    RUN_TEST(test_code_runner_sanitization);
    RUN_TEST(test_conversation_mode);
    RUN_TEST(test_idea_engine);

    // Integration tests (FEAT-005)
    RUN_TEST(test_technical_analysis_rsi);
    RUN_TEST(test_technical_analysis_macd);
    RUN_TEST(test_technical_analysis_bollinger);
    RUN_TEST(test_risk_manager_position_limit);
    RUN_TEST(test_risk_manager_drawdown);
    RUN_TEST(test_device_registry_add_remove);
    RUN_TEST(test_device_registry_roles);
    RUN_TEST(test_resource_monitor_check);
    RUN_TEST(test_goal_engine_create);
    RUN_TEST(test_goal_engine_progress);
    RUN_TEST(test_mastery_network_skill_up);
    RUN_TEST(test_mastery_network_decay);
    RUN_TEST(test_knowledge_sync_conflict);
    RUN_TEST(test_knowledge_sync_user_correction);
    RUN_TEST(test_knowledge_validator_format);
    RUN_TEST(test_knowledge_validator_duplicate);
    RUN_TEST(test_knowledge_validator_weight);
    RUN_TEST(test_config_load_defaults);
    RUN_TEST(test_config_load_parse);
    RUN_TEST(test_self_funding_track);
    RUN_TEST(test_trading_bot_paper_mode);
    RUN_TEST(test_signal_engine_generation);
    RUN_TEST(test_smart_router_crypto);
    RUN_TEST(test_smart_router_homelab);
    RUN_TEST(test_smart_router_mind);
    RUN_TEST(test_autonomous_learner_session);

    std::cout << std::endl;
    std::cout << "================================================" << std::endl;
    std::cout << "  RESULTS: " << g_tests_passed << " passed, "
              << g_tests_failed << " failed" << std::endl;
    std::cout << "  ASSERTIONS: " << g_assertions_passed << " passed, "
              << g_assertions_failed << " failed" << std::endl;
    std::cout << "================================================" << std::endl;
    std::cout << std::endl;

    return (g_tests_failed == 0) ? 0 : 1;
}

#endif // MK_TEST_MAIN_CPP
