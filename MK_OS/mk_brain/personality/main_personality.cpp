#include <iostream>
#include <string>
#include "personality_core.cpp"

int main() {
    std::cout << "[NEXUS CORE] Running Personality Core Validation Rig...\n\n";

    MKPersonalityCore personality;

    // 1. Check identity parameters
    std::cout << "\n[IDENTITY CHECK] System Identity: " << personality.getIdentity() << '\n';

    // 2. Simulate an incoming user question prompt
    std::string promptBuffer = "[USER]: How should we compile the final matrix layers?";

    // 3. Inject behavioral traits into the stream buffer
    personality.applyDirectives(promptBuffer);

    std::cout << "\n[PIPELINE OUTPUT] Final Context-Injected String Payload:\n";
    std::cout << promptBuffer << '\n';

    std::cout << "[NEXUS CORE] Personality layout validation complete.\n";
    return 0;
}