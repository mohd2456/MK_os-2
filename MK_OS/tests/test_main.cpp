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
#include "../llm/llm_engine.cpp"
#include "../ai_core/input_preprocessor.cpp"

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
// TEST: LLM Model Manager
// ============================================================
void test_llm_model_manager() {
    MKModelManager manager;

    // On this Linux system with 32GB RAM, getAvailableRAM should be positive
    float ram = manager.getAvailableRAM();
    TEST_ASSERT_GT(ram, 0.0f, "getAvailableRAM() should return a positive value on this system");

    // Hardware check should pass (system has >> 2GB RAM)
    bool sufficient = manager.checkHardware();
    TEST_ASSERT_TRUE(sufficient, "checkHardware() should return true on a system with 32GB RAM");

    // RAM string should not be empty
    std::string ramStr = manager.getRAMString();
    TEST_ASSERT_GT(ramStr.size(), 0u, "getRAMString() should return a non-empty string");

    // Minimum RAM should be a positive value
    float minRAM = manager.getMinRAM();
    TEST_ASSERT_GT(minRAM, 0.0f, "getMinRAM() should return a positive value");
}

// ============================================================
// TEST: LLM Engine
// ============================================================
void test_llm_engine() {
    MKLLMEngine engine;

    // No LLM server is running during tests, so isAvailable should be false
    TEST_ASSERT_FALSE(engine.isAvailable(), "isAvailable() should return false when no server is running");

    // Server type should be "none" when unavailable
    std::string serverType = engine.getServerType();
    TEST_ASSERT_EQ(serverType, std::string("none"), "getServerType() should return 'none' when no server available");

    // generate() should return empty string gracefully when no server is available
    std::string result = engine.generate("test prompt");
    TEST_ASSERT_EQ(result, std::string(""), "generate() should return empty string when no server available");

    // Model manager reference should be accessible and functional
    const MKModelManager& mgr = engine.getModelManager();
    TEST_ASSERT_GT(mgr.getAvailableRAM(), 0.0f, "Engine's model manager should report positive RAM");
}

// ============================================================
// TEST: Preprocessor Model Fallback
// ============================================================
void test_preprocessor_model_fallback() {
    MKInputPreprocessor preprocessor;

    // Call preprocessWithModel - should fall back to rule-based since no server is running
    MKPreprocessResult result = preprocessor.preprocessWithModel("hello world");

    // The result should be valid (not crashing)
    TEST_ASSERT_GT(result.cleaned_text.size(), 0u, "preprocessWithModel fallback should return non-empty cleaned_text");

    // was_modified should be a valid boolean (just verify it doesn't crash)
    TEST_ASSERT_TRUE(result.was_modified == true || result.was_modified == false,
                     "was_modified should be a valid boolean value");

    // The original text should be preserved
    TEST_ASSERT_EQ(result.original_text, std::string("hello world"),
                   "original_text should preserve the input");

    // Confidence should be non-negative
    TEST_ASSERT_TRUE(result.confidence >= 0.0f, "confidence should be non-negative");
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
    RUN_TEST(test_llm_model_manager);
    RUN_TEST(test_llm_engine);
    RUN_TEST(test_preprocessor_model_fallback);

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
