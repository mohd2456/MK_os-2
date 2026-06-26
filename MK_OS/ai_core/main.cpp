#ifndef MK_MAIN_CPP
#define MK_MAIN_CPP

#include <iostream>
#include <string>
#include <vector>

// Core AI modules
#include "memory.cpp"
#include "reasoning.cpp"
#include "language.cpp"
#include "execution.cpp"
#include "feedback.cpp"
#include "coding.cpp"
#include "writing.cpp"
#include "nexus.cpp"      // Nexus optimization subsystem included

// Dataset modules
#include "../datasets/loader.cpp"
#include "../datasets/parser.cpp"
#include "../datasets/db.cpp"
#include "../datasets/compressor.cpp"

int main() {
    std::cout << "==================================================\n";
    std::cout << "[SYSTEM] MK_OS Initializing on Core Hardware Matrix...\n";
    std::cout << "==================================================\n";
    std::cout << "[MEMORY] Allocated 2,048 token local sliding window budget.\n";

    // Core AI module instances
    MKMemory memory;
    MKReasoning reasoning;
    MKLanguage language;
    MKExecution execution;
    MKFeedback feedback;
    MKCoding coding;
    MKWriting writing;
    MKNexus nexus;         // Nexus hardware manager instantiated

    // Dataset module instances
    MKDatasetLoader loader;
    MKDatasetParser parser;
    MKDatasetDB db;
    MKCompressor compressor;

    // Load datasets safely from your storage folders
    auto mathData = loader.loadTXT("datasets/math/formulas.txt");
    auto slangData = parser.parseCSV("datasets/talk/slang.csv");
    auto jsonData = parser.parseJSON("datasets/knowledge/facts.json");
    
    db.openDB("datasets/code/codebase.db");
    compressor.compress("datasets/math/formulas.txt");
    compressor.decompress("datasets/math/formulas.txt");

    // Store some dataset entries in active short-term memory
    if (!mathData.empty()) {
        memory.store("math_example", mathData[0]);
    }
    if (!slangData.empty()) {
        memory.store("slang_example", slangData.begin()->first + "=" + slangData.begin()->second);
    }
    if (!jsonData.empty()) {
        memory.store("json_example", jsonData.begin()->first + "=" + jsonData.begin()->second);
    }

    // --- PILLAR 9 HEURISTIC BYPASS TEST ---
    std::cout << "\n[NEXUS CHECK] Testing intercept routing engine...\n";
    std::string testQuery = "What is the system specs and current time?";
    std::string rapidResponse;
    
    if (nexus.runHeuristicRouting(testQuery, rapidResponse)) {
        std::cout << "MK (Bypassed Neural Core) -> " << rapidResponse << "\n";
    }

    // Execution Core Demonstrations
    std::cout << "\n==================================================\n";
    std::cout << "MK> Reasoning: " << reasoning.evaluateExpression("(3+4)*2") << "\n";
    std::cout << "MK> Language: " << language.respond("Knowledge loaded", "technical") << "\n";

    // Run the autonomous build tests
    coding.writeCode("test.cpp", "#include <iostream>\nint main(){std::cout<<\"Hello from MK datasets!\";}");
    std::cout << "MK> Compiler Status: " << coding.compileAndRun("test.cpp") << "\n";

    std::cout << "MK> Writer Engine: " << writing.generateEssay("Artificial Intelligence with MK datasets") << "\n";
    std::cout << "==================================================\n\n";

    // Save session logs back to database
    memory.saveToFile("mk_memory.db");
    memory.status();

    // Clear Nexus working tensor memory bounds safely
    nexus.clearWorkingCache();

    std::cout << "[SYSTEM] Execution cycle completed cleanly.\n";
    return 0;
}

#endif // MK_MAIN_CPP