#include <iostream>
#include <string>
#include "reasoning_engine.cpp"

int main() {
    std::cout << "[NEXUS CORE] Running Reasoning Engine Validation Rig...\n\n";

    MKReasoningEngine engine;

    std::string prompt1 = "Hello Nexus, check system uptime.";
    std::string prompt2 = "Analyze memory leaks and compile the master matrix modules.";

    // 1. Test Prompt 1 (Simple Command)
    std::cout << "\n[ROUTE EVALUATION] Processing Prompt 1: \"" << prompt1 << "\"\n";
    if (engine.evaluateComplexity(prompt1) == InferencePath::CASCADE_THINK) {
        engine.executeThinkChain(prompt1);
    } else {
        std::cout << "[ROUTE EXECUTION] -> Fast Track Route Selected (Low CPU Cost).\n";
    }

    // 2. Test Prompt 2 (Complex Request)
    std::cout << "\n[ROUTE EVALUATION] Processing Prompt 2: \"" << prompt2 << "\"\n";
    if (engine.evaluateComplexity(prompt2) == InferencePath::CASCADE_THINK) {
        std::cout << "[ROUTE EXECUTION] -> Cascade Think Route Selected (High Priority Stack).\n";
        engine.executeThinkChain(prompt2);
    } else {
        std::cout << "[ROUTE EXECUTION] -> Fast Track Route Selected.\n";
    }

    std::cout << "\n[NEXUS CORE] Reasoning architecture validation complete.\n";
    return 0;
}