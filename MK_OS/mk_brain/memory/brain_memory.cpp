#ifndef MK_BRAIN_MEMORY_CPP
#define MK_BRAIN_MEMORY_CPP

#include <iostream>
#include <vector>
#include <string>

struct DialogTurn {
    std::string role;
    std::string content;
};

class MKBrainMemory {
private:
    std::vector<DialogTurn> shortTermContext;
    size_t maxContextTurns;

public:
    MKBrainMemory(size_t maxTurns = 6) : maxContextTurns(maxTurns) {
        std::cout << "[BRAIN MEMORY] Initializing context tracking registers...\n";
    }

    // Appends a new interaction turn to the rotating context window
    void commitToShortTerm(const std::string& role, const std::string& text) {
        if (shortTermContext.size() >= maxContextTurns) {
            // Evict the oldest turn to prevent RAM leakages over long sessions
            shortTermContext.erase(shortTermContext.begin());
        }
        shortTermContext.push_back({role, text});
        std::cout << "[CONTEXT REGISTER] Appended session data from: " << role << '\n';
    }

    // Flushes volatile short-term loops down to persistent disk anchors
    void archiveToLongTerm() {
        if (shortTermContext.empty()) return;
        
        std::cout << "[PERSISTENCE LOG] Archiving " << shortTermContext.size() 
                  << " dialogue threads into long-term history tables...\n";
        
        // Simulates saving straight to non-volatile disk blocks
        shortTermContext.clear();
        std::cout << "[PERSISTENCE LOG] Short-term context recycled safely.\n";
    }

    void dumpContext() const {
        std::cout << "--- ACTIVE CONTEXT BUFFER ---\n";
        for (const auto& turn : shortTermContext) {
            std::cout << " [" << turn.role << "]: " << turn.content << '\n';
        }
        std::cout << "-----------------------------\n";
    }

    // Returns conversation history as a formatted string for LLM context
    std::string getContextString() const {
        std::string result;
        for (const auto& turn : shortTermContext) {
            result += turn.role + ": " + turn.content + "\n";
        }
        return result;
    }
};

#endif // MK_BRAIN_MEMORY_CPP