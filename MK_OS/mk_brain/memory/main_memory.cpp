#include <iostream>
#include "brain_memory.cpp"

int main() {
    std::cout << "[NEXUS CORE] Running Brain Memory Validation Rig...\n\n";

    // Initialize with a small max size of 3 to quickly verify the rolling eviction logic
    MKBrainMemory memory(3);

    // 1. Fill the context buffer past its threshold
    memory.commitToShortTerm("user", "Hello, Nexus.");
    memory.commitToShortTerm("assistant", "Greetings. Subsystems optimal.");
    memory.commitToShortTerm("user", "What is my current battery health?");
    
    std::cout << "\n[SYSTEM LOG] Initial 3 entries loaded:\n";
    memory.dumpContext();

    // 2. Add another entry to force the oldest ("Hello, Nexus.") out of the array
    std::cout << "\n[SYSTEM LOG] Pushing 4th entry to trigger rolling eviction...\n";
    memory.commitToShortTerm("assistant", "Battery is at 61% charge.");
    memory.dumpContext();

    // 3. Archive context to long-term disk and clear the active registers
    std::cout << "\n[SYSTEM LOG] Committing active session to storage tables...\n";
    memory.archiveToLongTerm();

    std::cout << "\n[NEXUS CORE] Memory tracking layer verification complete.\n";
    return 0;
}