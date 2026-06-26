#ifndef MK_MAIN_SCHEDULER_CPP
#define MK_MAIN_SCHEDULER_CPP

#include <iostream>
#include <string>
#include "mk_scheduler.cpp"
#include "cpu.cpp"

int main() {
    std::cout << "==================================================\n";
    std::cout << "[NEXUS CORE] Booting Process Scheduler Test Rig\n";
    std::cout << "==================================================\n\n";

    MKScheduler scheduler;
    MKCPU cpu;

    // Registering diverse task loads with priority weight rankings
    scheduler.addTask({"Answer casual chat", 1});
    scheduler.addTask({"Solve algebra equation", 2});
    scheduler.addTask({"Compile C++ program", 2});
    scheduler.addTask({"Check slang dictionary", 1});

    std::cout << "\n[SCHEDULER] Dispatching tasks to native thread loops:\n";
    std::cout << "--------------------------------------------------\n";
    scheduler.run();
    std::cout << "--------------------------------------------------\n";

    std::cout << "\n[NEXUS CORE] Task scheduling validation complete.\n";
    std::cout << "==================================================\n";

    return 0;
}

#endif // MK_MAIN_SCHEDULER_CPP