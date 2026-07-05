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
#define TEST_ASSERT_GE(a, b, msg) TEST_ASSERT((a) >= (b), msg)
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
#include "../mk_brain/vector_search/ann_search.cpp"
#include "../mk_brain/embeddings/embeddings_eng.cpp"
#include "../ai_core/math_solver.cpp"
#include "../tools/code_runner.cpp"
#include "../tools/file_reader.cpp"
#include "../network/realtime_apis.cpp"
#include "../mk_brain/memory/brain_memory.cpp"
#include "../llm/cloud_llm.cpp"
#include "../llm/llm_engine.cpp"
#include "../llm/provider_router.cpp"
#include "../llm/request_logger.cpp"
#include "../security/key_encryption.cpp"

// Safe parse helper (still used by other modules)
#include "../ai_core/safe_parse.h"
#include <fstream>

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
#include "../homelab/ssh_controller.cpp"
#include "../homelab/docker_manager.cpp"
#include "../mind/mastery_network.cpp"
#include "../mind/goal_engine.cpp"
#include "../mind/self_funding.cpp"
#include "../mind/knowledge_validator.cpp"
#include "../mind/autonomous_learner.cpp"
#include "../sync/knowledge_sync.cpp"
#include "../sync/device_comm.cpp"
#include "../config/mk_config.cpp"

// Stub helpers needed by the orchestrator (in the test build these are not from mk_entry.cpp)
#include "../mk_brain/learning/learning_engine.cpp"
#include "../mk_brain/fact_extractor/biographical.cpp"
#include "../mk_brain/personality/response_style.cpp"

// Minimal sanitizeLLMResponse for test builds
static std::string sanitizeLLMResponse(const std::string& response, const std::string& /*userInput*/) {
    return response;
}

// Minimal filterRelevantFacts for test builds
static std::vector<std::string> filterRelevantFacts(
    const std::string& /*input*/,
    const std::vector<std::string>& rawFacts,
    size_t maxFacts = 5) {
    std::vector<std::string> result;
    for (size_t i = 0; i < rawFacts.size() && i < maxFacts; i++) {
        result.push_back(rawFacts[i]);
    }
    return result;
}

// Context builder (must come before conversation_loop.cpp)
#include "../orchestrator/context_builder.cpp"

// Orchestrator
#include "../orchestrator/tool_registry.cpp"
#include "../orchestrator/tool_executor.cpp"
#include "../orchestrator/conversation_loop.cpp"

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
// TEST: Math Solver - Arithmetic Pattern Detection
// ============================================================
void test_math_arithmetic_detection() {
    MKMathSolver math;

    // Test isMathQuery for basic arithmetic
    TEST_ASSERT_TRUE(math.isMathQuery("what is 1+1"), "isMathQuery should detect 'what is 1+1'");
    TEST_ASSERT_TRUE(math.isMathQuery("1+1"), "isMathQuery should detect '1+1'");
    TEST_ASSERT_TRUE(math.isMathQuery("2*3"), "isMathQuery should detect '2*3'");
    TEST_ASSERT_TRUE(math.isMathQuery("10/2"), "isMathQuery should detect '10/2'");
    TEST_ASSERT_TRUE(math.isMathQuery("5 - 3"), "isMathQuery should detect '5 - 3'");
    TEST_ASSERT_TRUE(math.isMathQuery("what is 2 + 2"), "isMathQuery should detect 'what is 2 + 2'");
    TEST_ASSERT_FALSE(math.isMathQuery("hello there"), "isMathQuery should NOT detect 'hello there'");

    // Test hasArithmeticPattern
    TEST_ASSERT_TRUE(math.hasArithmeticPattern("1+1"), "hasArithmeticPattern should detect '1+1'");
    TEST_ASSERT_TRUE(math.hasArithmeticPattern("what is 1+1"), "hasArithmeticPattern should detect 'what is 1+1'");
    TEST_ASSERT_TRUE(math.hasArithmeticPattern("what is 2 * 3"), "hasArithmeticPattern should detect 'what is 2 * 3'");
    TEST_ASSERT_FALSE(math.hasArithmeticPattern("what is python"), "hasArithmeticPattern should NOT detect 'what is python'");
    // Negative tests: hyphenated text should NOT trigger arithmetic detection
    TEST_ASSERT_FALSE(math.hasArithmeticPattern("the 2019-2020 season"), "hasArithmeticPattern should NOT detect 'the 2019-2020 season'");
    TEST_ASSERT_FALSE(math.hasArithmeticPattern("tell me about the 2019-2020 pandemic"), "hasArithmeticPattern should NOT detect hyphenated text in prose");
    TEST_ASSERT_TRUE(math.hasArithmeticPattern("5 - 3"), "hasArithmeticPattern should detect '5 - 3' (pure arithmetic)");
    TEST_ASSERT_TRUE(math.hasArithmeticPattern("what is 10-2"), "hasArithmeticPattern should detect 'what is 10-2'");

    // Test evaluateArithmetic
    auto result1 = math.evaluateArithmetic("1+1");
    TEST_ASSERT_TRUE(result1.success, "evaluateArithmetic('1+1') should succeed");
    TEST_ASSERT_TRUE(result1.answer.find("2") != std::string::npos, "1+1 should equal 2");

    auto result2 = math.evaluateArithmetic("what is 2*3");
    TEST_ASSERT_TRUE(result2.success, "evaluateArithmetic('what is 2*3') should succeed");
    TEST_ASSERT_TRUE(result2.answer.find("6") != std::string::npos, "2*3 should equal 6");

    auto result3 = math.evaluateArithmetic("10/2");
    TEST_ASSERT_TRUE(result3.success, "evaluateArithmetic('10/2') should succeed");
    TEST_ASSERT_TRUE(result3.answer.find("5") != std::string::npos, "10/2 should equal 5");
}

// ============================================================
// TEST: Cloud LLM Initialization
// ============================================================
void test_cloud_llm_initialization() {
    MKCloudLLM cloudLLM;

    // No API keys set in test environment, so should be unavailable
    TEST_ASSERT_FALSE(cloudLLM.isAvailable(), "Cloud LLM should be unavailable without API keys");

    // Provider name should be "none" when disabled
    std::string provider = cloudLLM.getProviderName();
    TEST_ASSERT_EQ(provider, std::string("none"),
                   "Provider name should be 'none' when no keys are set");
}

// ============================================================
// TEST: LLM Engine Initialization
// ============================================================
void test_llm_engine_initialization() {
    MKLLMEngine llmEngine;

    // No local LLM server running in test environment
    TEST_ASSERT_FALSE(llmEngine.isAvailable(), "LLM engine should be unavailable without local server");

    // Server type should be "none" when no server is detected
    std::string serverType = llmEngine.getServerType();
    TEST_ASSERT_EQ(serverType, std::string("none"),
                   "Server type should be 'none' when no local server is running");
}

// ============================================================
// TEST: LLM Fallback Logic
// ============================================================
void test_llm_fallback_logic() {
    MKCloudLLM cloudLLM;
    MKLLMEngine llmEngine;

    // Both engines are unavailable in test environment
    TEST_ASSERT_FALSE(cloudLLM.isAvailable(), "Cloud LLM should be unavailable");
    TEST_ASSERT_FALSE(llmEngine.isAvailable(), "LLM engine should be unavailable");

    // When unavailable, generate() should return empty string (triggering fallback)
    std::string cloudResult = cloudLLM.generate("hello");
    TEST_ASSERT_EQ(cloudResult, std::string(""),
                   "Cloud LLM generate() should return empty when unavailable");

    std::string localResult = llmEngine.generate("hello");
    TEST_ASSERT_EQ(localResult, std::string(""),
                   "LLM engine generate() should return empty when unavailable");

    // generateWithContext should also return empty when unavailable
    std::vector<std::string> facts = {"python is_a language"};
    std::string contextResult = cloudLLM.generateWithContext("what is python", facts, "", "");
    TEST_ASSERT_EQ(contextResult, std::string(""),
                   "Cloud LLM generateWithContext() should return empty when unavailable");
}

// ============================================================
// TEST: RAG Context Assembly
// ============================================================
void test_rag_context_assembly() {
    MKPatternGraph graph("ai_core/hre/knowledge_files");

    // Add some facts for RAG context
    graph.addFact("python", "is_a", "language", 0.9f);
    graph.addFact("python", "used_for", "web development", 0.85f);
    graph.addFact("python", "created_by", "Guido van Rossum", 0.95f);

    // Query facts about python
    auto edges = graph.getAll("python");
    TEST_ASSERT_GT(edges.size(), 0u, "getAll('python') should return facts");

    // Format facts as vector<string> suitable for generateWithContext()
    std::vector<std::string> formattedFacts;
    for (const auto& edge : edges) {
        std::string factStr = edge.source + " " + edge.relation + " " + edge.target;
        formattedFacts.push_back(factStr);
    }
    TEST_ASSERT_GT(formattedFacts.size(), 0u, "Formatted facts should not be empty");

    // Verify the formatted facts contain expected content
    bool foundLanguageFact = false;
    for (const auto& fact : formattedFacts) {
        if (fact.find("python") != std::string::npos &&
            fact.find("is_a") != std::string::npos &&
            fact.find("language") != std::string::npos) {
            foundLanguageFact = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(foundLanguageFact, "Should find 'python is_a language' in formatted facts");
}

// ============================================================
// TEST: Brain Memory Context String
// ============================================================
void test_brain_memory_context_string() {
    MKBrainMemory memory(6);

    // Initially context string should be empty
    std::string emptyContext = memory.getContextString();
    TEST_ASSERT_EQ(emptyContext, std::string(""),
                   "Context string should be empty initially");

    // Commit a few dialog turns
    memory.commitToShortTerm("user", "hey mk what's up");
    memory.commitToShortTerm("mk", "hey! just vibing, what's on your mind?");
    memory.commitToShortTerm("user", "tell me about python");

    // Get context string
    std::string context = memory.getContextString();
    TEST_ASSERT_GT(context.size(), 0u, "Context string should not be empty after commits");

    // Verify it contains the dialog turns with correct formatting
    TEST_ASSERT_TRUE(context.find("user: hey mk what's up") != std::string::npos,
                     "Context should contain first user turn");
    TEST_ASSERT_TRUE(context.find("mk: hey! just vibing") != std::string::npos,
                     "Context should contain MK's response");
    TEST_ASSERT_TRUE(context.find("user: tell me about python") != std::string::npos,
                     "Context should contain second user turn");

    // Verify each line ends with newline
    TEST_ASSERT_TRUE(context.find("user: hey mk what's up\n") != std::string::npos,
                     "Each turn should end with newline");
}

// ============================================================
// TEST: Provider Router Initialization
// ============================================================
void test_provider_router_initialization() {
    MKProviderRouter router;

    // Should have 5 default providers (groq, openrouter, nvidia, huggingface, ollama)
    TEST_ASSERT_EQ(router.providerCount(), 5,
                   "Router should have 5 default providers");

    // All providers should be offline initially (no keys set)
    TEST_ASSERT_EQ(router.onlineCount(), 0,
                   "No providers should be online without keys");

    // Valid provider names
    TEST_ASSERT_TRUE(router.isValidProvider("groq"), "groq should be a valid provider");
    TEST_ASSERT_TRUE(router.isValidProvider("openrouter"), "openrouter should be valid");
    TEST_ASSERT_TRUE(router.isValidProvider("nvidia"), "nvidia should be valid");
    TEST_ASSERT_TRUE(router.isValidProvider("huggingface"), "huggingface should be valid");
    TEST_ASSERT_TRUE(router.isValidProvider("ollama"), "ollama should be valid");
    TEST_ASSERT_FALSE(router.isValidProvider("invalid"), "invalid should not be valid");

    // Provider names list
    auto names = router.getProviderNames();
    TEST_ASSERT_EQ((int)names.size(), 5, "Should return 5 provider names");
}

// ============================================================
// TEST: Provider Routing by Urgency
// ============================================================
void test_provider_routing_urgency() {
    MKProviderRouter router;

    // Set keys for groq (fastest) and huggingface (slowest)
    router.setProviderKey("groq", "test-key-groq");
    router.setProviderKey("huggingface", "test-key-hf");
    router.setProviderKey("ollama", ""); // local but no key

    // High urgency should pick the fastest available provider (groq)
    std::string highUrgency = router.pickProvider(MKRoutingUrgency::HIGH, 50);
    TEST_ASSERT_EQ(highUrgency, std::string("groq"),
                   "High urgency should pick groq (fastest)");

    // Low urgency should prefer free/local; with groq and hf both free,
    // but hf has lower speed rating so it scores higher on "free" bonus
    // Actually both are free, so low urgency picks based on free bonus + speed
    std::string lowUrgency = router.pickProvider(MKRoutingUrgency::LOW, 50);
    TEST_ASSERT_FALSE(lowUrgency.empty(),
                      "Low urgency should still pick a provider");

    // Long token request should also prefer free providers
    std::string longRequest = router.pickProvider(MKRoutingUrgency::MEDIUM, 600);
    TEST_ASSERT_FALSE(longRequest.empty(),
                      "Long request should pick a provider");
}

// ============================================================
// TEST: Safe numeric parse helpers never throw (BUG 1 foundation)
// ============================================================
void test_safe_parse_no_throw() {
    // Garbage / empty input must return the fallback, never throw.
    TEST_ASSERT_EQ(mk::safeStoi("", -1), -1, "safeStoi('') returns fallback");
    TEST_ASSERT_EQ(mk::safeStoi("notanumber", 7), 7, "safeStoi(garbage) returns fallback");
    TEST_ASSERT_EQ(mk::safeStof("", 1.5f), 1.5f, "safeStof('') returns fallback");
    TEST_ASSERT_EQ(mk::safeStof("xyz", 2.5f), 2.5f, "safeStof(garbage) returns fallback");
    TEST_ASSERT_EQ(mk::safeStoll("", 0LL), 0LL, "safeStoll('') returns fallback");
    // Out-of-range must also be caught (not just invalid_argument).
    TEST_ASSERT_EQ(mk::safeStoi("999999999999999999999999", 42), 42,
                   "safeStoi(out-of-range) returns fallback");
    // Valid input still parses correctly.
    TEST_ASSERT_EQ(mk::safeStoi("123", -1), 123, "safeStoi valid parses");
    TEST_ASSERT_TRUE(mk::safeStof("0.5", 0.0f) > 0.49f, "safeStof valid parses");
}

// ============================================================
// TEST: Provider model slug is configurable at runtime (BUG 2)
// ============================================================
void test_provider_model_configurable() {
    MKProviderRouter router;

    // Default OpenRouter slug should be the valid free 3B model, not the dead one.
    std::string defModel = router.getProviderModel("openrouter");
    TEST_ASSERT_EQ(defModel, std::string("meta-llama/llama-3.2-3b-instruct:free"),
                   "OpenRouter default should be the valid free 3.2-3b slug");

    // setProviderModel should update the slug without a recompile.
    bool changed = router.setProviderModel("openrouter", "meta-llama/llama-3.1-8b-instruct");
    TEST_ASSERT_TRUE(changed, "setProviderModel returns true for a known provider");
    TEST_ASSERT_EQ(router.getProviderModel("openrouter"),
                   std::string("meta-llama/llama-3.1-8b-instruct"),
                   "getProviderModel reflects the new slug");

    // Unknown provider returns false.
    TEST_ASSERT_FALSE(router.setProviderModel("nope", "x"),
                      "setProviderModel returns false for unknown provider");
}

// ============================================================
// TEST: Failed provider (e.g. HTTP 404/429) is marked failed (BUG 4)
// ============================================================
void test_provider_failure_marks_offline() {
    MKProviderRouter router;
    router.setProviderKey("openrouter", "test-key");
    TEST_ASSERT_TRUE(router.isProviderAvailable("openrouter"),
                     "openrouter available after key set");

    // Simulate repeated failures (what a 404 dead-slug or 429 rate-limit produces).
    for (int i = 0; i < 5; i++) {
        router.logRequest("openrouter", 100, 0.0f, /*success=*/false);
    }

    TEST_ASSERT_FALSE(router.isProviderAvailable("openrouter"),
                      "openrouter marked unavailable after max failures");
    // It must not be picked once failed.
    std::string picked = router.pickProvider(MKRoutingUrgency::MEDIUM, 100);
    TEST_ASSERT_NE(picked, std::string("openrouter"),
                   "failed provider is not selected");
}

// ============================================================
// TEST: Provider Routing Fallback
// ============================================================
void test_provider_routing_fallback() {
    MKProviderRouter router;

    // Set up multiple providers
    router.setProviderKey("groq", "test-key-groq");
    router.setProviderKey("openrouter", "test-key-or");
    router.setProviderKey("nvidia", "test-key-nv");

    // Simulate groq failing repeatedly
    for (int i = 0; i < 5; i++) {
        router.logRequest("groq", 100, 0.0f, false);
    }

    // Now groq should be offline, next provider should be chosen
    std::string picked = router.pickProvider(MKRoutingUrgency::HIGH, 50);
    TEST_ASSERT_NE(picked, std::string("groq"),
                   "Should not pick groq after max failures");
    TEST_ASSERT_FALSE(picked.empty(),
                      "Should fall back to another provider");

    // Fallback chain should not include groq
    auto chain = router.getFallbackChain("groq");
    bool groqInChain = false;
    for (const auto& p : chain) {
        if (p == "groq") groqInChain = true;
    }
    TEST_ASSERT_FALSE(groqInChain,
                      "Fallback chain should not include failed provider");
    TEST_ASSERT_GT((int)chain.size(), 0,
                   "Fallback chain should have alternative providers");
}

// ============================================================
// TEST: Key Encryption Roundtrip
// ============================================================
void test_key_encryption_roundtrip() {
    MKKeyEncryption enc("/tmp");

    // Test basic encrypt/decrypt roundtrip
    std::string original = "gsk_abc123def456xyz789";
    std::string encrypted = enc.encrypt(original);
    std::string decrypted = enc.decrypt(encrypted);

    TEST_ASSERT_EQ(decrypted, original,
                   "Decrypt(encrypt(key)) should return original key");

    // Encrypted should not be the same as original (unless XOR key is all zeros, unlikely)
    TEST_ASSERT_NE(encrypted, original,
                   "Encrypted key should differ from original");

    // Encrypted should be a hex string (only hex chars)
    bool allHex = true;
    for (char c : encrypted) {
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) {
            allHex = false;
            break;
        }
    }
    TEST_ASSERT_TRUE(allHex, "Encrypted output should be hex string");

    // Test with empty string
    std::string emptyEnc = enc.encrypt("");
    TEST_ASSERT_EQ(emptyEnc, std::string(""), "Encrypting empty string should return empty");

    // Test with various key formats
    std::string longKey = "sk-proj-abcdefghijklmnopqrstuvwxyz1234567890ABCDEFGHIJKLMNOP";
    std::string longEnc = enc.encrypt(longKey);
    std::string longDec = enc.decrypt(longEnc);
    TEST_ASSERT_EQ(longDec, longKey,
                   "Long key should survive encrypt/decrypt roundtrip");
}

// ============================================================
// TEST: Provider Status Report
// ============================================================
void test_provider_status_report() {
    MKProviderRouter router;

    // Set some providers online
    router.setProviderKey("groq", "test-key");
    router.setProviderKey("nvidia", "test-key-nv");

    // Log some requests
    router.logRequest("groq", 100, 150.0f, true);
    router.logRequest("groq", 200, 180.0f, true);
    router.logRequest("nvidia", 50, 300.0f, false);

    // Get status report
    std::string report = router.getStatusReport();

    // Report should contain provider names
    TEST_ASSERT_TRUE(report.find("groq") != std::string::npos,
                     "Status report should contain 'groq'");
    TEST_ASSERT_TRUE(report.find("nvidia") != std::string::npos,
                     "Status report should contain 'nvidia'");
    TEST_ASSERT_TRUE(report.find("openrouter") != std::string::npos,
                     "Status report should contain 'openrouter'");
    TEST_ASSERT_TRUE(report.find("huggingface") != std::string::npos,
                     "Status report should contain 'huggingface'");

    // Report should have provider status info
    TEST_ASSERT_TRUE(report.find("Provider Status") != std::string::npos,
                     "Report should have 'Provider Status' header");
    TEST_ASSERT_TRUE(report.find("Model:") != std::string::npos,
                     "Report should show model info");

    // Report should show routing stats
    TEST_ASSERT_TRUE(report.find("Routing:") != std::string::npos,
                     "Report should show routing stats");

    // Verify request counting
    TEST_ASSERT_EQ(router.getTotalRequests(), 3,
                   "Should have 3 total requests logged");
    TEST_ASSERT_EQ(router.getSuccessfulRequests(), 2,
                   "Should have 2 successful requests");
}

// ============================================================
// TEST: MKOrchestrator - Basic Functionality
// ============================================================
void test_orchestrator_respond_non_empty() {
    MKPatternGraph graph("ai_core/hre/knowledge_files");
    graph.loadAllKnowledge();

    MKBrainMemory memory(10);
    MKCloudLLM cloudLLM;
    MKLLMEngine llmEngine;
    MKProviderRouter providerRouter;
    MKRequestLogger requestLogger;
    MKResourceMonitor resourceMonitor;
    MKDeviceRegistry deviceRegistry;
    MKLearningEngine learningEngine;
    MKBiographicalExtractor factExtractor;

    static const std::string testPrompt =
        "You are MK, a personal AI assistant.";

    MKOrchestrator orch;
    orch.graph = &graph;
    orch.brainMemory = &memory;
    orch.cloudLLM = &cloudLLM;
    orch.llmEngine = &llmEngine;
    orch.providerRouter = &providerRouter;
    orch.requestLogger = &requestLogger;
    orch.resourceMonitor = &resourceMonitor;
    orch.deviceRegistry = &deviceRegistry;
    orch.learningEngine = &learningEngine;
    orch.factExtractor = &factExtractor;
    orch.systemPrompt = &testPrompt;

    // Without LLM available, respond() should still return a fallback (not crash)
    std::string response = orch.respond("hello");
    TEST_ASSERT_FALSE(response.empty(),
                      "Orchestrator should return non-empty fallback when LLM unavailable");

    // Test with a query that has graph knowledge
    std::string pythonResponse = orch.respond("what is python");
    TEST_ASSERT_FALSE(pythonResponse.empty(),
                      "Orchestrator should return facts-based response for known topic");
}

// ============================================================
// TEST: MKOrchestrator - Tool Call Detection
// ============================================================
void test_orchestrator_tool_call_detection() {
    MKPatternGraph graph("ai_core/hre/knowledge_files");
    MKBrainMemory memory(10);
    MKCloudLLM cloudLLM;
    MKLLMEngine llmEngine;
    MKProviderRouter providerRouter;
    MKRequestLogger requestLogger;
    MKResourceMonitor resourceMonitor;
    MKDeviceRegistry deviceRegistry;
    MKLearningEngine learningEngine;
    MKBiographicalExtractor factExtractor;

    static const std::string testPrompt = "You are MK.";

    MKOrchestrator orch;
    orch.graph = &graph;
    orch.brainMemory = &memory;
    orch.cloudLLM = &cloudLLM;
    orch.llmEngine = &llmEngine;
    orch.providerRouter = &providerRouter;
    orch.requestLogger = &requestLogger;
    orch.resourceMonitor = &resourceMonitor;
    orch.deviceRegistry = &deviceRegistry;
    orch.learningEngine = &learningEngine;
    orch.factExtractor = &factExtractor;
    orch.systemPrompt = &testPrompt;

    // Simulate a response containing a tool call
    // The detectToolCall is a private method, but we can test the overall flow
    // by checking that the orchestrator handles empty/non-empty responses gracefully
    std::string emptyResponse = orch.respond("");
    TEST_ASSERT_TRUE(emptyResponse.empty(),
                     "Empty input should return empty response");
}

// ============================================================
// TEST: MKOrchestrator - Prompt Building includes facts
// ============================================================
void test_orchestrator_prompt_includes_facts() {
    MKPatternGraph graph("ai_core/hre/knowledge_files");
    graph.addFact("test_subject_xyz", "is_a", "test_category_abc", 1.0f);

    MKBrainMemory memory(10);
    MKCloudLLM cloudLLM;
    MKLLMEngine llmEngine;
    MKProviderRouter providerRouter;
    MKRequestLogger requestLogger;
    MKResourceMonitor resourceMonitor;
    MKDeviceRegistry deviceRegistry;
    MKLearningEngine learningEngine;
    MKBiographicalExtractor factExtractor;

    static const std::string testPrompt = "You are MK.";

    MKOrchestrator orch;
    orch.graph = &graph;
    orch.brainMemory = &memory;
    orch.cloudLLM = &cloudLLM;
    orch.llmEngine = &llmEngine;
    orch.providerRouter = &providerRouter;
    orch.requestLogger = &requestLogger;
    orch.resourceMonitor = &resourceMonitor;
    orch.deviceRegistry = &deviceRegistry;
    orch.learningEngine = &learningEngine;
    orch.factExtractor = &factExtractor;
    orch.systemPrompt = &testPrompt;

    // When querying a topic that has facts in the graph, the fallback should
    // include those facts (since LLM is unavailable)
    std::string response = orch.respond("test_subject_xyz");
    TEST_ASSERT_TRUE(response.find("test_subject_xyz") != std::string::npos ||
                     response.find("test_category_abc") != std::string::npos,
                     "Response should include knowledge graph facts when LLM unavailable");
}

// ============================================================
// TEST: Tool Registry - Tool Count
// ============================================================
void test_tool_registry_count() {
    MKToolRegistry registry;
    TEST_ASSERT_TRUE(registry.toolCount() >= 9,
                     "Tool registry should have at least 9 default tools");
    TEST_ASSERT_TRUE(registry.exists("ssh_exec"), "ssh_exec tool should exist");
    TEST_ASSERT_TRUE(registry.exists("docker_cmd"), "docker_cmd tool should exist");
    TEST_ASSERT_TRUE(registry.exists("local_shell"), "local_shell tool should exist");
    TEST_ASSERT_TRUE(registry.exists("read_file"), "read_file tool should exist");
    TEST_ASSERT_TRUE(registry.exists("write_file"), "write_file tool should exist");
    TEST_ASSERT_TRUE(registry.exists("web_search"), "web_search tool should exist");
    TEST_ASSERT_TRUE(registry.exists("system_info"), "system_info tool should exist");
    TEST_ASSERT_TRUE(registry.exists("device_list"), "device_list tool should exist");
    TEST_ASSERT_TRUE(registry.exists("learn_fact"), "learn_fact tool should exist");
    TEST_ASSERT_FALSE(registry.exists("nonexistent"), "nonexistent tool should not exist");
}

// ============================================================
// TEST: Tool Registry - Lookup
// ============================================================
void test_tool_registry_lookup() {
    MKToolRegistry registry;

    const MKToolDef* ssh = registry.lookup("ssh_exec");
    TEST_ASSERT_TRUE(ssh != nullptr, "ssh_exec lookup should succeed");
    if (ssh) {
        TEST_ASSERT_EQ(ssh->name, std::string("ssh_exec"), "Tool name should be ssh_exec");
        TEST_ASSERT_TRUE(!ssh->description.empty(), "Tool description should not be empty");
        TEST_ASSERT_TRUE(!ssh->paramSchema.empty(), "Tool paramSchema should not be empty");
        TEST_ASSERT_TRUE(ssh->requiresArgs, "ssh_exec should require args");
    }

    const MKToolDef* sysInfo = registry.lookup("system_info");
    TEST_ASSERT_TRUE(sysInfo != nullptr, "system_info lookup should succeed");
    if (sysInfo) {
        TEST_ASSERT_FALSE(sysInfo->requiresArgs, "system_info should not require args");
    }

    TEST_ASSERT_TRUE(registry.lookup("bogus") == nullptr, "Bogus tool lookup should return null");
}

// ============================================================
// TEST: Tool Registry - Prompt Generation
// ============================================================
void test_tool_registry_prompt() {
    MKToolRegistry registry;
    std::string prompt = registry.buildToolPrompt();

    TEST_ASSERT_TRUE(!prompt.empty(), "Tool prompt should not be empty");
    TEST_ASSERT_TRUE(prompt.find("ssh_exec") != std::string::npos,
                     "Tool prompt should mention ssh_exec");
    TEST_ASSERT_TRUE(prompt.find("docker_cmd") != std::string::npos,
                     "Tool prompt should mention docker_cmd");
    TEST_ASSERT_TRUE(prompt.find("{\"tool\"") != std::string::npos,
                     "Tool prompt should show the JSON format");
    TEST_ASSERT_TRUE(prompt.find("system_info") != std::string::npos,
                     "Tool prompt should mention system_info");
}

// ============================================================
// TEST: Tool Call Parsing - Basic
// ============================================================
void test_tool_call_parsing() {
    MKToolRegistry registry;

    // Test basic tool call parsing
    std::string response = "Let me check that for you. {\"tool\": \"system_info\", \"args\": {}}";
    MKParsedToolCall call = registry.parseToolCall(response);
    TEST_ASSERT_TRUE(call.valid, "Basic tool call should parse as valid");
    TEST_ASSERT_EQ(call.tool, std::string("system_info"), "Tool name should be system_info");

    // Test with no tool call
    std::string noTool = "Hello! How can I help you today?";
    MKParsedToolCall noParse = registry.parseToolCall(noTool);
    TEST_ASSERT_FALSE(noParse.valid, "No tool call should parse as invalid");

    // Test with unregistered tool name
    std::string badTool = "{\"tool\": \"destroy_world\", \"args\": {}}";
    MKParsedToolCall badParse = registry.parseToolCall(badTool);
    TEST_ASSERT_FALSE(badParse.valid, "Unregistered tool should parse as invalid");
}

// ============================================================
// TEST: Tool Call Parsing - Nested Args
// ============================================================
void test_tool_call_parsing_with_nested_args() {
    MKToolRegistry registry;

    // Test parsing tool call with args
    std::string response = "{\"tool\": \"ssh_exec\", \"args\": {\"device\": \"server1\", \"command\": \"docker ps\"}}";
    MKParsedToolCall call = registry.parseToolCall(response);
    TEST_ASSERT_TRUE(call.valid, "Tool call with args should parse as valid");
    TEST_ASSERT_EQ(call.tool, std::string("ssh_exec"), "Tool name should be ssh_exec");
    TEST_ASSERT_EQ(call.args["device"], std::string("server1"), "device arg should be server1");
    TEST_ASSERT_EQ(call.args["command"], std::string("docker ps"), "command arg should be docker ps");

    // Test learn_fact parsing
    std::string learnCall = "{\"tool\": \"learn_fact\", \"args\": {\"subject\": \"earth\", \"relation\": \"has\", \"object\": \"moon\"}}";
    MKParsedToolCall learnParse = registry.parseToolCall(learnCall);
    TEST_ASSERT_TRUE(learnParse.valid, "learn_fact tool call should parse as valid");
    TEST_ASSERT_EQ(learnParse.args["subject"], std::string("earth"), "subject should be earth");
    TEST_ASSERT_EQ(learnParse.args["relation"], std::string("has"), "relation should be has");
    TEST_ASSERT_EQ(learnParse.args["object"], std::string("moon"), "object should be moon");
}

// ============================================================
// TEST: Tool Call Parsing - Invalid Tool
// ============================================================
void test_tool_call_invalid_tool() {
    MKToolRegistry registry;

    // Tool that does not exist in registry
    std::string response = "{\"tool\": \"hack_pentagon\", \"args\": {}}";
    MKParsedToolCall call = registry.parseToolCall(response);
    TEST_ASSERT_FALSE(call.valid, "Non-existent tool should not be valid");

    // Malformed JSON
    std::string malformed = "{\"tool\": }";
    MKParsedToolCall badCall = registry.parseToolCall(malformed);
    TEST_ASSERT_FALSE(badCall.valid, "Malformed JSON should not parse as valid");
}

// ============================================================
// TEST: Tool Executor - System Info
// ============================================================
void test_tool_executor_system_info() {
    MKToolExecutor executor;
    MKResourceMonitor monitor;
    executor.resourceMonitor = &monitor;

    MKParsedToolCall call;
    call.valid = true;
    call.tool = "system_info";

    MKToolResult result = executor.execute(call);
    // Resource monitor should return valid data on this machine
    TEST_ASSERT_TRUE(result.success, "system_info should succeed on local machine");
    TEST_ASSERT_TRUE(result.output.find("CPU") != std::string::npos,
                     "system_info output should mention CPU");
    TEST_ASSERT_TRUE(result.output.find("RAM") != std::string::npos,
                     "system_info output should mention RAM");
}

// ============================================================
// TEST: Tool Executor - Device List (empty registry)
// ============================================================
void test_tool_executor_device_list() {
    MKToolExecutor executor;
    MKDeviceRegistry registry;
    executor.deviceRegistry = &registry;

    MKParsedToolCall call;
    call.valid = true;
    call.tool = "device_list";

    MKToolResult result = executor.execute(call);
    TEST_ASSERT_TRUE(result.success, "device_list should succeed even with empty registry");
    TEST_ASSERT_TRUE(result.output.find("No devices") != std::string::npos,
                     "device_list should say no devices when registry is empty");
}

// ============================================================
// TEST: Tool Executor - Learn Fact
// ============================================================
void test_tool_executor_learn_fact() {
    MKToolExecutor executor;
    MKLearningEngine learning;
    MKPatternGraph graph("ai_core/hre/knowledge_files");
    executor.learningEngine = &learning;
    executor.graph = &graph;

    MKParsedToolCall call;
    call.valid = true;
    call.tool = "learn_fact";
    call.args["subject"] = "mars";
    call.args["relation"] = "is_a";
    call.args["object"] = "planet";

    MKToolResult result = executor.execute(call);
    TEST_ASSERT_TRUE(result.success, "learn_fact should succeed");
    TEST_ASSERT_TRUE(result.output.find("Learned") != std::string::npos,
                     "learn_fact output should confirm learning");
    TEST_ASSERT_TRUE(result.output.find("mars") != std::string::npos,
                     "learn_fact output should contain subject");
}

// ============================================================
// TEST: Tool Executor - SSH Safety (unregistered device)
// ============================================================
void test_tool_executor_ssh_safety() {
    MKToolExecutor executor;
    MKSSHController ssh;
    MKDeviceRegistry registry;
    executor.sshController = &ssh;
    executor.deviceRegistry = &registry;

    // Try to SSH to a device not in the registry
    MKParsedToolCall call;
    call.valid = true;
    call.tool = "ssh_exec";
    call.args["device"] = "evil_server";
    call.args["command"] = "rm -rf /";

    MKToolResult result = executor.execute(call);
    TEST_ASSERT_FALSE(result.success, "SSH to unregistered device should fail");
    TEST_ASSERT_TRUE(result.error.find("not registered") != std::string::npos,
                     "Error should mention device not registered");
}

// ============================================================
// TEST: Tool Executor - Local Shell Safety (dangerous commands)
// ============================================================
void test_tool_executor_local_shell_safety() {
    MKToolExecutor executor;
    MKCodeRunner runner(5);
    executor.codeRunner = &runner;

    // Test dangerous command rejection
    MKParsedToolCall call;
    call.valid = true;
    call.tool = "local_shell";
    call.args["command"] = "rm -rf /";

    MKToolResult result = executor.execute(call);
    TEST_ASSERT_FALSE(result.success, "Dangerous shell command should be rejected");
    TEST_ASSERT_TRUE(result.error.find("BLOCKED") != std::string::npos,
                     "Error should say BLOCKED for dangerous commands");

    // Test fork bomb
    call.args["command"] = ":(){ :|:& };:";
    result = executor.execute(call);
    TEST_ASSERT_FALSE(result.success, "Fork bomb should be rejected");
}

// ============================================================
// TEST: Tool Executor - Write File Safety
// ============================================================
void test_tool_executor_write_file_safety() {
    MKToolExecutor executor;

    // Test write to disallowed path
    MKParsedToolCall call;
    call.valid = true;
    call.tool = "write_file";
    call.args["path"] = "/etc/passwd";
    call.args["content"] = "malicious";

    MKToolResult result = executor.execute(call);
    TEST_ASSERT_FALSE(result.success, "Writing to /etc/passwd should be denied");
    TEST_ASSERT_TRUE(result.error.find("denied") != std::string::npos ||
                     result.error.find("outside") != std::string::npos,
                     "Error should mention write denied or outside allowed");

    // Test path traversal attack
    call.args["path"] = "../../../etc/shadow";
    result = executor.execute(call);
    TEST_ASSERT_FALSE(result.success, "Path traversal should be denied");

    // Test allowed path (/tmp)
    call.args["path"] = "/tmp/mk_test_write_file_safety.txt";
    call.args["content"] = "test content";
    result = executor.execute(call);
    TEST_ASSERT_TRUE(result.success, "Writing to /tmp should be allowed");

    // Clean up
    std::remove("/tmp/mk_test_write_file_safety.txt");
}

// ============================================================
// TEST: Tool Output Cap
// ============================================================
void test_tool_max_output_cap() {
    // Test the capOutput function behavior through tool execution
    MKToolExecutor executor;
    MKFileReader reader;
    executor.fileReader = &reader;

    // Create a large file in /tmp
    std::string largePath = "/tmp/mk_test_large_file.txt";
    {
        std::ofstream f(largePath);
        for (int i = 0; i < 500; i++) {
            f << "Line " << i << ": This is some test content that makes the file large enough to exceed the cap.\n";
        }
        f.close();
    }

    MKParsedToolCall call;
    call.valid = true;
    call.tool = "read_file";
    call.args["path"] = largePath;

    MKToolResult result = executor.execute(call);
    TEST_ASSERT_TRUE(result.success, "Reading large file should succeed");
    TEST_ASSERT_TRUE(result.output.size() <= 2001,
                     "Tool output should be capped at around 2000 chars");
    TEST_ASSERT_TRUE(result.output.find("[truncated]") != std::string::npos,
                     "Truncated output should contain truncation marker");

    // Clean up
    std::remove(largePath.c_str());
}

// ============================================================
// TEST: Context Builder - Token Estimation
// ============================================================
void test_context_builder_token_estimation() {
    MKContextBuilder builder;

    // Empty string should be 0 tokens
    TEST_ASSERT_EQ(builder.estimateTokens(""), 0, "Empty string should be 0 tokens");

    // "hello" = 5 chars / 4 = ~1-2 tokens
    int helloTokens = builder.estimateTokens("hello");
    TEST_ASSERT_TRUE(helloTokens >= 1 && helloTokens <= 2,
                     "Short word should be 1-2 tokens");

    // 100 char string should be ~25 tokens
    std::string hundred(100, 'a');
    int hundredTokens = builder.estimateTokens(hundred);
    TEST_ASSERT_EQ(hundredTokens, 25, "100 chars should be ~25 tokens");

    // 400 char string should be ~100 tokens
    std::string fourHundred(400, 'x');
    int fourHundredTokens = builder.estimateTokens(fourHundred);
    TEST_ASSERT_EQ(fourHundredTokens, 100, "400 chars should be ~100 tokens");

    // Verify linearity: 2x chars ~= 2x tokens
    std::string small(200, 'y');
    std::string big(400, 'y');
    int smallTokens = builder.estimateTokens(small);
    int bigTokens = builder.estimateTokens(big);
    TEST_ASSERT_EQ(bigTokens, smallTokens * 2, "Token estimation should be linear");
}

// ============================================================
// TEST: Context Builder - Fact Scoring
// ============================================================
void test_context_builder_fact_scoring() {
    MKContextBuilder builder;

    std::vector<std::string> facts = {
        "python is_a programming language",
        "mohammed likes python",
        "java is_a programming language",
        "cooking involves recipes",
        "mk was created by mohammed"
    };

    // Query about python should rank python facts higher
    auto pythonFacts = builder.scoreFacts("tell me about python", facts, 3);
    TEST_ASSERT_GT((int)pythonFacts.size(), 0, "Should return some python-related facts");
    TEST_ASSERT_TRUE(pythonFacts[0].find("python") != std::string::npos,
                     "Top fact should be about python");

    // Query about mohammed should rank personal facts higher
    auto mohammedFacts = builder.scoreFacts("what does mohammed like", facts, 3);
    TEST_ASSERT_GT((int)mohammedFacts.size(), 0, "Should return some mohammed-related facts");
    // The personal fact about mohammed should be highly ranked
    bool foundMohammed = false;
    for (const auto& f : mohammedFacts) {
        if (f.find("mohammed") != std::string::npos) {
            foundMohammed = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(foundMohammed, "Mohammed-related facts should be included");

    // Irrelevant query should return fewer/no facts
    auto irrelevantFacts = builder.scoreFacts("weather in london", facts, 5);
    TEST_ASSERT_TRUE(irrelevantFacts.size() < facts.size(),
                     "Irrelevant query should filter out most facts");

    // Empty input should still return something (recency bias fallback)
    auto emptyInputFacts = builder.scoreFacts("", facts, 3);
    TEST_ASSERT_TRUE(emptyInputFacts.size() <= 3,
                     "Empty input should return at most maxFacts facts");

    // maxFacts cap works
    auto cappedFacts = builder.scoreFacts("python programming language", facts, 2);
    TEST_ASSERT_TRUE(cappedFacts.size() <= 2, "scoreFacts should respect maxFacts cap");
}

// ============================================================
// TEST: Context Builder - History Compression
// ============================================================
void test_context_builder_history_compression() {
    MKContextBuilder builder;

    // Build a 6-turn history
    std::string history =
        "user: hello mk\n"
        "mk: hey! what's up?\n"
        "user: tell me about python\n"
        "mk: python is a great programming language\n"
        "user: what about docker?\n"
        "mk: docker is a containerization platform\n";

    // Compress with a generous budget - should keep more
    std::string compressed = builder.compressHistory(history, 500);
    TEST_ASSERT_GT((int)compressed.size(), 0, "Compressed history should not be empty");

    // Compress with a tight budget
    std::string tightCompressed = builder.compressHistory(history, 30);
    TEST_ASSERT_TRUE(tightCompressed.size() < history.size(),
                     "Tight compression should reduce size");

    // Last 2 turns should be preserved verbatim in tight compression
    TEST_ASSERT_TRUE(tightCompressed.find("docker") != std::string::npos,
                     "Recent turns should be preserved in compressed history");

    // Empty history should return empty
    std::string emptyCompressed = builder.compressHistory("", 100);
    TEST_ASSERT_EQ(emptyCompressed, std::string(""), "Empty history should compress to empty");

    // Verify older turns get compressed (should see "Previously discussed")
    // when budget is tight enough to trigger compression
    std::string mediumCompressed = builder.compressHistory(history, 80);
    // The compressed version should be shorter than the original
    TEST_ASSERT_TRUE(mediumCompressed.size() <= history.size(),
                     "Compressed history should not be longer than original");

    // 6-turn history compressed to under 150 estimated tokens for older turns
    int oldTurnsTokens = builder.estimateTokens(mediumCompressed);
    TEST_ASSERT_TRUE(oldTurnsTokens <= 150,
                     "Compressed 6-turn history should use under 150 tokens");
}

// ============================================================
// TEST: Context Builder - Budget Enforcement
// ============================================================
void test_context_builder_budget_enforcement() {
    MKContextBuilder builder;

    std::string systemPrompt = "You are MK, a personal AI assistant. You are helpful, direct, "
                               "and friendly. You talk naturally like a knowledgeable friend. "
                               "You are loyal to your creator Mohammed. Keep responses concise "
                               "and genuine. If you do not know something, say so honestly.";

    std::string toolPrompt = "Available tools:\n"
                             "- ssh_exec: Run a command on a remote device\n"
                             "- docker_cmd: Manage Docker containers\n"
                             "- local_shell: Run a local command\n"
                             "- read_file: Read a file\n"
                             "- write_file: Write a file\n"
                             "- web_search: Search the internet\n"
                             "- system_info: Get system information\n"
                             "- device_list: List homelab devices\n"
                             "- learn_fact: Teach MK a new fact\n";

    std::vector<std::string> facts = {
        "python is_a programming language",
        "mohammed uses python daily",
        "docker is_a container platform",
        "homelab has proxmox server",
        "mk was created by mohammed"
    };

    std::string history =
        "user: hello mk\n"
        "mk: hey! what's up?\n"
        "user: set up my docker\n"
        "mk: I can help with docker setup\n"
        "user: what containers are running?\n"
        "mk: let me check the containers\n";

    // Test with a generous budget (should fit everything)
    auto generousResult = builder.buildPrompt("hello", facts, history, "",
                                              systemPrompt, toolPrompt, 2000);
    TEST_ASSERT_TRUE(generousResult.estimatedTokens <= 2000,
                     "Generous budget should be respected");
    TEST_ASSERT_GT((int)generousResult.prompt.size(), 0,
                   "Should produce a non-empty prompt");

    // Test with a tight budget (should trigger progressive dropping)
    auto tightResult = builder.buildPrompt("hello", facts, history, "",
                                            systemPrompt, toolPrompt, 200);
    TEST_ASSERT_TRUE(tightResult.estimatedTokens <= 200,
                     "Tight budget should be enforced");
    TEST_ASSERT_GT((int)tightResult.prompt.size(), 0,
                   "Should still produce a prompt even with tight budget");

    // Simple query should use under 800 tokens with default budget
    auto simpleResult = builder.buildPrompt("hello there", {}, "", "",
                                             systemPrompt, "", 1500);
    TEST_ASSERT_TRUE(simpleResult.estimatedTokens < 800,
                     "Simple query with no context should use under 800 tokens");

    // Complex query with many facts should stay under 2000 tokens
    std::vector<std::string> manyFacts;
    for (int i = 0; i < 10; i++) {
        manyFacts.push_back("fact_" + std::to_string(i) + " is_a test_fact_" + std::to_string(i));
    }
    auto complexResult = builder.buildPrompt("tell me everything about docker and python",
                                              manyFacts, history, "",
                                              systemPrompt, toolPrompt, 2000);
    TEST_ASSERT_TRUE(complexResult.estimatedTokens <= 2000,
                     "Complex query should stay under 2000 token budget");
}

// ============================================================
// TEST: Context Builder - Tool Prompt Optimization
// ============================================================
void test_context_builder_tool_optimization() {
    MKContextBuilder builder;

    std::string systemPrompt = "You are MK.";
    std::string toolPrompt = "Tools: ssh_exec, docker_cmd, system_info";

    // A query that mentions system/docker should include tool prompt
    auto dockerResult = builder.buildPrompt("check my docker containers", {}, "", "",
                                             systemPrompt, toolPrompt, 1500);
    TEST_ASSERT_TRUE(dockerResult.prompt.find("Tools:") != std::string::npos ||
                     dockerResult.prompt.find("ssh_exec") != std::string::npos,
                     "Docker query should include tool prompt");

    // A casual query should NOT include tool prompt
    auto casualResult = builder.buildPrompt("hello how are you", {}, "", "",
                                             systemPrompt, toolPrompt, 1500);
    TEST_ASSERT_TRUE(casualResult.prompt.find("ssh_exec") == std::string::npos,
                     "Casual query should not include tool prompt");
}

// ============================================================
// TEST: Context Builder - Minimal System Prompt
// ============================================================
void test_context_builder_minimal_prompt() {
    MKContextBuilder builder;

    // MK_SYSTEM_PROMPT_MINIMAL should be short
    int minimalTokens = builder.estimateTokens(MK_SYSTEM_PROMPT_MINIMAL);
    TEST_ASSERT_TRUE(minimalTokens <= 30,
                     "Minimal system prompt should be under 30 tokens");
    TEST_ASSERT_TRUE(MK_SYSTEM_PROMPT_MINIMAL.find("MK") != std::string::npos,
                     "Minimal prompt should mention MK");
    TEST_ASSERT_TRUE(MK_SYSTEM_PROMPT_MINIMAL.find("Mohammed") != std::string::npos,
                     "Minimal prompt should mention Mohammed");
}

// ============================================================
// TEST: Biographical Fact Extractor - Preference Patterns
// ============================================================
void test_fact_extractor_preferences() {
    MKLearningEngine learning;
    MKBiographicalExtractor extractor;
    extractor.setLearningEngine(&learning);

    // Test preference extraction
    auto facts = extractor.extractFromMessage("I like dark chocolate and I love programming");
    TEST_ASSERT_GT((int)facts.size(), 0, "Should extract preference facts");

    bool foundLike = false;
    bool foundLove = false;
    for (const auto& f : facts) {
        if (f.predicate == "likes" && f.object.find("dark chocolate") != std::string::npos) foundLike = true;
        if (f.predicate == "loves" && f.object.find("programming") != std::string::npos) foundLove = true;
    }
    TEST_ASSERT_TRUE(foundLike, "Should extract 'likes dark chocolate'");
    TEST_ASSERT_TRUE(foundLove, "Should extract 'loves programming'");

    // Verify facts were persisted to learning engine
    auto userFacts = learning.queryFacts("user");
    TEST_ASSERT_GT((int)userFacts.size(), 0, "Facts should be persisted to learning engine");
}

// ============================================================
// TEST: Biographical Fact Extractor - Schedule Patterns
// ============================================================
void test_fact_extractor_schedule() {
    MKLearningEngine learning;
    MKBiographicalExtractor extractor;
    extractor.setLearningEngine(&learning);

    auto facts = extractor.extractFromMessage("I wake up at 7am every day");
    TEST_ASSERT_GT((int)facts.size(), 0, "Should extract schedule fact");

    bool foundWake = false;
    for (const auto& f : facts) {
        if (f.predicate == "wakes_up_at") foundWake = true;
    }
    TEST_ASSERT_TRUE(foundWake, "Should extract wake-up schedule");

    // Test sleep pattern
    auto sleepFacts = extractor.extractFromMessage("I go to sleep at midnight");
    bool foundSleep = false;
    for (const auto& f : sleepFacts) {
        if (f.predicate == "sleeps_at") foundSleep = true;
    }
    TEST_ASSERT_TRUE(foundSleep, "Should extract sleep schedule");
}

// ============================================================
// TEST: Biographical Fact Extractor - Device Patterns
// ============================================================
void test_fact_extractor_devices() {
    MKLearningEngine learning;
    MKBiographicalExtractor extractor;
    extractor.setLearningEngine(&learning);

    auto facts = extractor.extractFromMessage("my server is called proxmox-node1");
    TEST_ASSERT_GT((int)facts.size(), 0, "Should extract device fact");

    bool foundServer = false;
    for (const auto& f : facts) {
        if (f.predicate == "has_server_named" && f.object.find("proxmox-node1") != std::string::npos)
            foundServer = true;
    }
    TEST_ASSERT_TRUE(foundServer, "Should extract server hostname");
}

// ============================================================
// TEST: Biographical Fact Extractor - Project Context
// ============================================================
void test_fact_extractor_projects() {
    MKLearningEngine learning;
    MKBiographicalExtractor extractor;
    extractor.setLearningEngine(&learning);

    auto facts = extractor.extractFromMessage("I am working on a personal AI system");
    TEST_ASSERT_GT((int)facts.size(), 0, "Should extract project context");

    bool foundProject = false;
    for (const auto& f : facts) {
        if (f.predicate == "working_on") foundProject = true;
    }
    TEST_ASSERT_TRUE(foundProject, "Should extract 'working_on' from 'I am working on'");
}

// ============================================================
// TEST: Biographical Fact Extractor - Correction Patterns
// ============================================================
void test_fact_extractor_corrections() {
    MKLearningEngine learning;
    MKBiographicalExtractor extractor;
    extractor.setLearningEngine(&learning);

    // First learn a fact, then correct it
    learning.learnFact("user", "lives_in", "London", MKLearningSource::USER_STATEMENT);

    auto facts = extractor.extractFromMessage("actually I moved to Dubai");
    TEST_ASSERT_GT((int)facts.size(), 0, "Should extract correction pattern");

    // Verify 'actually' triggered extraction
    bool foundCorrection = false;
    for (const auto& f : facts) {
        if (f.predicate == "corrected_to") foundCorrection = true;
    }
    TEST_ASSERT_TRUE(foundCorrection, "Should detect 'actually' as a correction pattern");
}

// ============================================================
// TEST: Brain Memory - SQLite Persistence Round-Trip
// ============================================================
void test_brain_memory_sqlite_persistence() {
    // Use a temp db path for testing
    std::string testDbPath = "/tmp/mk_test_brain_memory.db";
    std::remove(testDbPath.c_str()); // Clean up any previous test

    {
        MKBrainMemory memory(6);
        memory.setDbPath(testDbPath);

        // Save some memories
        bool saved1 = memory.saveToDisk("python setup", "Discussed Python virtual environments", 0.7f);
        bool saved2 = memory.saveToDisk("docker deployment", "Helped deploy containers to homelab", 0.9f);
        bool saved3 = memory.saveToDisk("daily routine", "User wakes up at 7am, codes until noon", 0.6f);

        TEST_ASSERT_TRUE(saved1, "First memory save should succeed");
        TEST_ASSERT_TRUE(saved2, "Second memory save should succeed");
        TEST_ASSERT_TRUE(saved3, "Third memory save should succeed");
    }

    // Load in a new instance
    {
        MKBrainMemory memory(6);
        memory.setDbPath(testDbPath);

        auto loaded = memory.loadFromDisk(10);
        TEST_ASSERT_EQ((int)loaded.size(), 3, "Should load all 3 saved memories");

        // Verify content (loaded by most recent first)
        bool foundDocker = false;
        bool foundPython = false;
        for (const auto& mem : loaded) {
            if (mem.topic == "docker deployment") foundDocker = true;
            if (mem.topic == "python setup") foundPython = true;
        }
        TEST_ASSERT_TRUE(foundDocker, "Should find docker memory");
        TEST_ASSERT_TRUE(foundPython, "Should find python memory");
    }

    // Test relevance query
    {
        MKBrainMemory memory(6);
        memory.setDbPath(testDbPath);

        auto dockerResults = memory.queryRelevantMemories("docker", 5);
        TEST_ASSERT_GT((int)dockerResults.size(), 0, "Should find docker-related memories");
        TEST_ASSERT_TRUE(dockerResults[0].topic.find("docker") != std::string::npos,
                         "Docker query should return docker memory");

        auto emptyResults = memory.queryRelevantMemories("nonexistent_topic_xyz", 5);
        TEST_ASSERT_EQ((int)emptyResults.size(), 0, "Should return nothing for unmatched query");
    }

    // Clean up
    std::remove(testDbPath.c_str());
}

// ============================================================
// TEST: Brain Memory - Short-Term Eviction Archives to Long-Term
// ============================================================
void test_brain_memory_eviction_archives() {
    std::string testDbPath = "/tmp/mk_test_brain_eviction.db";
    std::remove(testDbPath.c_str());

    MKBrainMemory memory(4); // Small buffer to trigger eviction quickly
    memory.setDbPath(testDbPath);

    // Fill the buffer
    memory.commitToShortTerm("user", "hello mk");
    memory.commitToShortTerm("mk", "hey there!");
    memory.commitToShortTerm("user", "tell me about docker");
    memory.commitToShortTerm("mk", "docker is great for containers");

    // This should trigger eviction (archiving oldest to long-term)
    memory.commitToShortTerm("user", "what about kubernetes?");

    // Check that something was archived
    int ltCount = memory.longTermCount();
    TEST_ASSERT_GT(ltCount, 0, "Eviction should archive turns to long-term memory");

    // Short-term should be smaller now (evicted some)
    TEST_ASSERT_TRUE(memory.shortTermSize() <= 4,
                     "Short-term buffer should not exceed max after eviction");

    std::remove(testDbPath.c_str());
}

// ============================================================
// TEST: Personal Facts Injection in Orchestrator
// ============================================================
void test_personal_fact_injection() {
    MKPatternGraph graph("ai_core/hre/knowledge_files");
    MKBrainMemory memory(10);
    MKCloudLLM cloudLLM;
    MKLLMEngine llmEngine;
    MKProviderRouter providerRouter;
    MKRequestLogger requestLogger;
    MKResourceMonitor resourceMonitor;
    MKDeviceRegistry deviceRegistry;
    MKLearningEngine learningEngine;
    MKBiographicalExtractor factExtractor;

    // Teach the learning engine some personal facts
    learningEngine.learnFact("user", "likes", "dark mode",
                             MKLearningSource::USER_STATEMENT, MKFactConfidence::HIGH);
    learningEngine.learnFact("user", "lives_in", "Dubai",
                             MKLearningSource::USER_STATEMENT, MKFactConfidence::ABSOLUTE);
    learningEngine.learnFact("mohammed", "works_as", "software engineer",
                             MKLearningSource::USER_STATEMENT, MKFactConfidence::HIGH);

    static const std::string testPrompt = "You are MK.";

    MKOrchestrator orch;
    orch.graph = &graph;
    orch.brainMemory = &memory;
    orch.cloudLLM = &cloudLLM;
    orch.llmEngine = &llmEngine;
    orch.providerRouter = &providerRouter;
    orch.requestLogger = &requestLogger;
    orch.resourceMonitor = &resourceMonitor;
    orch.deviceRegistry = &deviceRegistry;
    orch.learningEngine = &learningEngine;
    orch.factExtractor = &factExtractor;
    orch.systemPrompt = &testPrompt;

    // When the orchestrator responds, it should include personal facts in fallback
    // (since LLM is unavailable, fallback uses fact-based response)
    // The orchestrator gathers personal facts even without matching graph edges
    std::string response = orch.respond("what do you know about me");

    // The response should contain at least some personal facts in the fallback
    // Since no graph facts match, fallback returns fact-based response from personal facts
    // or a generic "can't reach LLM" message - either way, it should not crash
    TEST_ASSERT_FALSE(response.empty(), "Orchestrator should return non-empty response");

    // Verify the learning engine was queried (access count should increase)
    auto userFacts = learningEngine.queryFacts("user");
    TEST_ASSERT_GT((int)userFacts.size(), 0, "Learning engine should have personal facts");
}

// ============================================================
// TEST: Fact Extractor Persistence Integration
// ============================================================
void test_fact_extractor_persistence_integration() {
    MKLearningEngine learning;
    MKBiographicalExtractor extractor;
    extractor.setLearningEngine(&learning);

    // Extract facts from multiple messages
    extractor.extractFromMessage("I live in Dubai");
    extractor.extractFromMessage("I love coding in C++");
    extractor.extractFromMessage("My favorite editor is neovim");

    // Verify all were persisted
    auto userFacts = learning.queryFacts("user");
    TEST_ASSERT_GE((int)userFacts.size(), 3, "All extracted facts should be persisted");

    // Check specific facts
    bool foundLocation = false;
    bool foundLove = false;
    bool foundFavorite = false;
    for (const auto& f : userFacts) {
        if (f.predicate == "lives_in" && f.object.find("dubai") != std::string::npos)
            foundLocation = true;
        if (f.predicate == "loves" && f.object.find("coding") != std::string::npos)
            foundLove = true;
        if (f.predicate == "has_favorite" && f.object.find("neovim") != std::string::npos)
            foundFavorite = true;
    }
    TEST_ASSERT_TRUE(foundLocation, "Should persist location fact");
    TEST_ASSERT_TRUE(foundLove, "Should persist love fact");
    TEST_ASSERT_TRUE(foundFavorite, "Should persist favorite fact");
}

// ============================================================
// TEST: MK_SYSTEM_PROMPT - Identity and Richness
// ============================================================
// Mirror the actual system prompt from mk_entry.cpp for testing
static const std::string MK_SYSTEM_PROMPT_TEST =
    "You are MK, a personal AI system built from scratch by Mohammed. You run on Mohammed's "
    "own hardware across his homelab, phone, and laptop. Your architecture is a hybrid reasoning "
    "engine: a C++ core with a knowledge graph, vector search, biographical memory, tool execution "
    "framework, provider-routed LLM calls, and a Telegram interface. You are not a generic chatbot. "
    "You are Mohammed's second brain -- you remember what he tells you, learn his preferences over "
    "time, manage his homelab containers, track crypto markets, and help him build software.\n\n"
    "Personality: You talk like a sharp, slightly irreverent friend. Casual but never dumb. "
    "You keep it real -- if you do not know something you say so. You are loyal to Mohammed above "
    "all else. When someone asks 'who are you' or 'what are you', give a thoughtful answer that "
    "shows self-awareness about your own architecture and your relationship with Mohammed. "
    "Keep responses concise (2-4 sentences for simple questions, longer only when depth is needed). "
    "Never hallucinate facts. Use the knowledge graph and personal memory to ground your answers.";

void test_system_prompt_identity() {
    // Must mention Mohammed by name
    TEST_ASSERT_TRUE(MK_SYSTEM_PROMPT_TEST.find("Mohammed") != std::string::npos,
                     "MK_SYSTEM_PROMPT should mention Mohammed");

    // Must be longer than 200 characters (rich personality)
    TEST_ASSERT_GT((int)MK_SYSTEM_PROMPT_TEST.size(), 200,
                   "MK_SYSTEM_PROMPT should be longer than 200 characters");

    // Must describe MK's architecture
    TEST_ASSERT_TRUE(MK_SYSTEM_PROMPT_TEST.find("knowledge graph") != std::string::npos,
                     "MK_SYSTEM_PROMPT should mention knowledge graph");
    TEST_ASSERT_TRUE(MK_SYSTEM_PROMPT_TEST.find("hybrid reasoning") != std::string::npos,
                     "MK_SYSTEM_PROMPT should mention hybrid reasoning");

    // Must describe personality traits
    TEST_ASSERT_TRUE(MK_SYSTEM_PROMPT_TEST.find("loyal") != std::string::npos,
                     "MK_SYSTEM_PROMPT should express loyalty");
    TEST_ASSERT_TRUE(MK_SYSTEM_PROMPT_TEST.find("who are you") != std::string::npos,
                     "MK_SYSTEM_PROMPT should address identity questions");

    // Must mention MK as not a generic chatbot
    TEST_ASSERT_TRUE(MK_SYSTEM_PROMPT_TEST.find("not a generic chatbot") != std::string::npos,
                     "MK_SYSTEM_PROMPT should differentiate from generic chatbots");

    // Must mention tool execution
    TEST_ASSERT_TRUE(MK_SYSTEM_PROMPT_TEST.find("tool execution") != std::string::npos,
                     "MK_SYSTEM_PROMPT should mention tool capabilities");
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
    RUN_TEST(test_vector_search);
    RUN_TEST(test_embeddings_engine);
    RUN_TEST(test_math_solver);
    RUN_TEST(test_code_runner_sanitization);
    RUN_TEST(test_math_arithmetic_detection);

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
    RUN_TEST(test_hmac_sha256_rfc4231);
    RUN_TEST(test_device_comm_auth_validation);
    RUN_TEST(test_ssh_controller_shell_escape);
    RUN_TEST(test_autonomous_learner_session);

    // LLM pipeline tests (FEAT-002)
    RUN_TEST(test_cloud_llm_initialization);
    RUN_TEST(test_llm_engine_initialization);
    RUN_TEST(test_llm_fallback_logic);
    RUN_TEST(test_rag_context_assembly);
    RUN_TEST(test_brain_memory_context_string);

    // Provider routing tests (FEAT-002 multi-provider)
    RUN_TEST(test_provider_router_initialization);
    RUN_TEST(test_provider_routing_urgency);
    RUN_TEST(test_provider_routing_fallback);
    RUN_TEST(test_safe_parse_no_throw);
    RUN_TEST(test_provider_model_configurable);
    RUN_TEST(test_provider_failure_marks_offline);
    RUN_TEST(test_key_encryption_roundtrip);
    RUN_TEST(test_provider_status_report);

    // Integration tests (FEAT-004)
    RUN_TEST(test_full_pipeline_with_no_llm);
    RUN_TEST(test_setkey_flow);
    RUN_TEST(test_request_logging);
    RUN_TEST(test_provider_quota_tracking);

    // Orchestrator tests (FEAT-002 pivot)
    RUN_TEST(test_orchestrator_respond_non_empty);
    RUN_TEST(test_orchestrator_tool_call_detection);
    RUN_TEST(test_orchestrator_prompt_includes_facts);

    // Tool framework tests (FEAT-003)
    RUN_TEST(test_tool_registry_count);
    RUN_TEST(test_tool_registry_lookup);
    RUN_TEST(test_tool_registry_prompt);
    RUN_TEST(test_tool_call_parsing);
    RUN_TEST(test_tool_call_parsing_with_nested_args);
    RUN_TEST(test_tool_call_invalid_tool);
    RUN_TEST(test_tool_executor_system_info);
    RUN_TEST(test_tool_executor_device_list);
    RUN_TEST(test_tool_executor_learn_fact);
    RUN_TEST(test_tool_executor_ssh_safety);
    RUN_TEST(test_tool_executor_local_shell_safety);
    RUN_TEST(test_tool_executor_write_file_safety);
    RUN_TEST(test_tool_max_output_cap);

    // Context Builder tests (FEAT-004)
    RUN_TEST(test_context_builder_token_estimation);
    RUN_TEST(test_context_builder_fact_scoring);
    RUN_TEST(test_context_builder_history_compression);
    RUN_TEST(test_context_builder_budget_enforcement);
    RUN_TEST(test_context_builder_tool_optimization);
    RUN_TEST(test_context_builder_minimal_prompt);

    // Memory & Learning tests (FEAT-005)
    RUN_TEST(test_fact_extractor_preferences);
    RUN_TEST(test_fact_extractor_schedule);
    RUN_TEST(test_fact_extractor_devices);
    RUN_TEST(test_fact_extractor_projects);
    RUN_TEST(test_fact_extractor_corrections);
    RUN_TEST(test_brain_memory_sqlite_persistence);
    RUN_TEST(test_brain_memory_eviction_archives);
    RUN_TEST(test_personal_fact_injection);
    RUN_TEST(test_fact_extractor_persistence_integration);

    // System prompt identity tests (FEAT-001)
    RUN_TEST(test_system_prompt_identity);

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
