#ifndef MK_SCHEDULER_CPP
#define MK_SCHEDULER_CPP

#include <iostream>
#include <queue>
#include <string>
#include <thread>
#include <chrono>
#include "task.cpp"

class MKScheduler {
private:
    std::queue<Task> fastQueue;
    std::queue<Task> deepQueue;

public:
    MKScheduler() {
        std::cout << "[SCHEDULER] Dual-queue priority matrix ready.\n";
    }

    // Sorts incoming structures into their target execution ring
    void addTask(const Task& t) {
        if (t.priority == 1) {
            fastQueue.push(t);
        } else {
            deepQueue.push(t);
        }
    }

    // Drains the priority arrays sequentially until execution complete
    void run() {
        while (!fastQueue.empty() || !deepQueue.empty()) {
            if (!fastQueue.empty()) {
                Task t = fastQueue.front(); 
                fastQueue.pop();
                std::cout << "[FAST MODE] Running high-priority thread: " << t.name << "\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            } else if (!deepQueue.empty()) {
                Task t = deepQueue.front(); 
                deepQueue.pop();
                std::cout << "[DEEP MODE] Running resource-heavy stack: " << t.name << "\n";
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
        }
    }
};

#endif // MK_SCHEDULER_CPP