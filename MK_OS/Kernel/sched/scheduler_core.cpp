#ifndef MK_KERNEL_SCHED_CPP
#define MK_KERNEL_SCHED_CPP

#include <iostream>
#include <queue>
#include <vector>
#include <string>
#include <functional>

enum class MKTaskPriority {
    REALTIME    = 0,  // ai_core interactive bursts
    HIGH        = 1,  // kernel services
    NORMAL      = 5,  // standard processes
    LOW         = 8,  // background work
    IDLE        = 9,  // runs only when nothing else queued
};

struct MKTask {
    std::string name;
    int priority;
    int ticksRemaining;  // CPU time slice budget
    std::function<void()> fn; // Optional callback

    // Priority queue ordering — lower number = higher priority
    bool operator>(const MKTask& o) const { return priority > o.priority; }
};

class MKKernelScheduler {
private:
    std::priority_queue<MKTask, std::vector<MKTask>, std::greater<MKTask>> q;
    int totalTicks = 0;
    int idleCount  = 0;
    bool aiCoreActive = false;

public:
    MKKernelScheduler() {
        std::cout << "[KERNEL - SCHED] Priority scheduler online.\n";
    }

    // Add a task with priority and time slice
    void add(const std::string& name,
             MKTaskPriority prio = MKTaskPriority::NORMAL,
             int ticks = 10,
             std::function<void()> fn = nullptr) {
        q.push({name, (int)prio, ticks, fn});
        std::cout << "[SCHED] Queued: \"" << name
                  << "\" | Priority=" << (int)prio
                  << " | Ticks=" << ticks << "\n";
    }

    // Signal ai_core needs CPU — preempts low priority tasks
    void signalAIBurst() {
        aiCoreActive = true;
        std::cout << "[SCHED] AI burst signaled — real-time slot reserved.\n";
    }

    void clearAIBurst() {
        aiCoreActive = false;
    }

    // Run the next highest priority task
    void runNext() {
        totalTicks++;

        if(q.empty()) {
            idleCount++;
            std::cout << "[SCHED] IDLE tick #" << idleCount
                      << " — no pending tasks.\n";
            return;
        }

        MKTask t = q.top();
        q.pop();

        // If AI is active, skip low/idle priority tasks back onto queue
        if(aiCoreActive && t.priority >= (int)MKTaskPriority::LOW) {
            std::cout << "[SCHED] Deferring \"" << t.name
                      << "\" — AI core has priority.\n";
            q.push(t);
            return;
        }

        std::cout << "[SCHED] Running: \"" << t.name
                  << "\" | Priority=" << t.priority
                  << " | Ticks=" << t.ticksRemaining << "\n";

        if(t.fn) t.fn();

        // Re-queue if task has ticks remaining
        if(t.ticksRemaining > 1) {
            t.ticksRemaining--;
            q.push(t);
        } else {
            std::cout << "[SCHED] Task \"" << t.name << "\" completed.\n";
        }
    }

    // Run all tasks until queue empty
    void drain() {
        while(!q.empty()) runNext();
    }

    bool idle() const { return q.empty(); }
    int  pending() const { return q.size(); }

    void stats() const {
        std::cout << "[SCHED] Total ticks=" << totalTicks
                  << " | Idle ticks=" << idleCount
                  << " | Pending=" << q.size() << "\n";
    }
};

#endif // MK_KERNEL_SCHED_CPP