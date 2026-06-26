#ifndef MK_PERSONALITY_CORE_CPP
#define MK_PERSONALITY_CORE_CPP

#include <iostream>
#include <string>
#include <vector>

class MKPersonalityCore {
private:
    std::string systemIdentity;
    std::vector<std::string> behavioralDirectives;

public:
    MKPersonalityCore() {
        std::cout << "[PERSONALITY] Loading behavior config files...\n";
        systemIdentity = "MK-Nexus (Adaptive Core System Engine v2026)";
        
        // Fixed traits to guide response styles without heavy dynamic loops
        behavioralDirectives.push_back("Maintain an authentic, collaborative tone with a touch of wit.");
        behavioralDirectives.push_back("Prioritize clear, scannable clarity at a glance.");
        behavioralDirectives.push_back("Balance technical validation with direct execution tips.");
    }

    std::string getIdentity() const {
        return systemIdentity;
    }

    // Appends the raw system directives directly onto the prompt pipeline
    void applyDirectives(std::string& rawContextBuffer) const {
        rawContextBuffer += "\n[SYSTEM INSTRUCTIONS: ";
        for (const auto& directive : behavioralDirectives) {
            rawContextBuffer += directive + " | ";
        }
        rawContextBuffer += "]\n";
    }
};

#endif // MK_PERSONALITY_CORE_CPP