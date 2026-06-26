#ifndef MK_TOKEN_SCHEDULER_CPP
#define MK_TOKEN_SCHEDULER_CPP

#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <functional>
#include <chrono>
#include <algorithm>

// ===================================================================================
// MK TOKEN-BASED REAL-TIME PRIORITY SCHEDULER
// Implements the dynamic token system from the spec:
//   - Tokens grant temporary real-time execution priority to ai_core
//   - Background tasks instantly yield when AI burst is signaled
//   - Cooperative priority yielding between subsystems
//   - Thermal-aware task mitigation
//   - Opportunistic AC-charging heavy task scheduling
// ===================================================================================

enum class MKTokenPriority {
    REALTIME    = 0,    // AI interactive burst — immediate execution
    ELEVATED    = 1,    // Kernel critical tasks
    NORMAL      = 5,    // Standard user-facing work
    BACKGROUND  = 8,    // Indexing, compaction, cleanup
    IDLE        = 9     // Only when absolutely nothing else to do
};

struct MKSchedulerToken {
    int tokenId;
    std::string owner;          // Which subsystem holds this token
    MKTokenPriority priority;
    int cpuSlicesGranted;       // How many time slices this token grants
    int cpuSlicesUsed;
    bool active;
    std::chrono::steady_clock::time_point grantedAt;
    std::chrono::steady_clock::time_point expiresAt;
};

struct MKScheduledTask {
    int taskId;
    std::string name;
    std::string subsystem;
    MKTokenPriority priority;
    int tokenId;                // Which token authorized this task
    int estimatedSlices;        // How long we think it'll take
    bool yielded;               // Voluntarily gave up CPU
    bool preempted;             // Forcibly removed for higher priority
    std::function<void()> execute;
    
    bool operator>(const MKScheduledTask& other) const {
        return (int)priority > (int)other.priority;
    }
};

class MKTokenScheduler {
private:
    // Token pool
    std::vector<MKSchedulerToken> tokenPool;
    int nextTokenId;
    int maxActiveTokens;
    
    // Task queue (min-heap by priority)
    std::priority_queue<MKScheduledTask, std::vector<MKScheduledTask>, 
                        std::greater<MKScheduledTask>> taskQueue;
    
    // Active state
    bool aiBurstActive;
    bool thermalThrottled;
    bool onBattery;
    float performanceScale;     // 0.0 to 1.0, from thermal monitor
    
    // Statistics
    int totalTasksRun;
    int totalYields;
    int totalPreemptions;
    int totalTokensGranted;
    int aiTokensGranted;
    
    // Deferred background tasks (yielded when AI burst fires)
    std::vector<MKScheduledTask> deferredTasks;

public:
    MKTokenScheduler(int maxTokens = 16) 
        : nextTokenId(0), maxActiveTokens(maxTokens), aiBurstActive(false),
          thermalThrottled(false), onBattery(false), performanceScale(1.0f),
          totalTasksRun(0), totalYields(0), totalPreemptions(0), 
          totalTokensGranted(0), aiTokensGranted(0) {
        std::cout << "[TOKEN SCHED] Real-time priority scheduler initialized. Max tokens: " 
                  << maxTokens << "\n";
    }
    
    // Grant a priority token to a subsystem
    int grantToken(const std::string& owner, MKTokenPriority priority, int slices = 10) {
        MKSchedulerToken token;
        token.tokenId = nextTokenId++;
        token.owner = owner;
        token.priority = priority;
        token.cpuSlicesGranted = slices;
        token.cpuSlicesUsed = 0;
        token.active = true;
        token.grantedAt = std::chrono::steady_clock::now();
        token.expiresAt = token.grantedAt + std::chrono::milliseconds(slices * 10);
        
        tokenPool.push_back(token);
        totalTokensGranted++;
        
        if (priority == MKTokenPriority::REALTIME) aiTokensGranted++;
        
        std::cout << "[TOKEN SCHED] Token #" << token.tokenId << " granted to \"" 
                  << owner << "\" | Priority=" << (int)priority 
                  << " | Slices=" << slices << "\n";
        return token.tokenId;
    }
    
    // Signal an AI interactive burst — preempts all background tasks
    void signalAIBurst(const std::string& reason = "user_interaction") {
        aiBurstActive = true;
        
        // Yield all background/idle tasks immediately
        std::priority_queue<MKScheduledTask, std::vector<MKScheduledTask>,
                            std::greater<MKScheduledTask>> newQueue;
        
        while (!taskQueue.empty()) {
            MKScheduledTask task = taskQueue.top();
            taskQueue.pop();
            
            if (task.priority >= MKTokenPriority::BACKGROUND) {
                task.yielded = true;
                deferredTasks.push_back(task);
                totalYields++;
            } else {
                newQueue.push(task);
            }
        }
        taskQueue = newQueue;
        
        // Grant real-time token to ai_core
        grantToken("ai_core", MKTokenPriority::REALTIME, 50);
        
        std::cout << "[TOKEN SCHED] AI BURST: Background tasks yielded. Reason: " << reason << "\n";
    }
    
    // Clear AI burst — re-queue deferred tasks
    void clearAIBurst() {
        aiBurstActive = false;
        
        for (auto& task : deferredTasks) {
            task.yielded = false;
            taskQueue.push(task);
        }
        deferredTasks.clear();
        
        std::cout << "[TOKEN SCHED] AI burst ended. Deferred tasks re-queued.\n";
    }
    
    // Submit a task for scheduling
    void submitTask(const std::string& name, const std::string& subsystem,
                    MKTokenPriority priority, int estimatedSlices = 5,
                    std::function<void()> fn = nullptr) {
        
        // If AI burst is active and this is background, defer immediately
        if (aiBurstActive && priority >= MKTokenPriority::BACKGROUND) {
            MKScheduledTask task;
            task.taskId = totalTasksRun + deferredTasks.size();
            task.name = name;
            task.subsystem = subsystem;
            task.priority = priority;
            task.tokenId = -1;
            task.estimatedSlices = estimatedSlices;
            task.yielded = true;
            task.preempted = false;
            task.execute = fn;
            deferredTasks.push_back(task);
            return;
        }
        
        // If thermal throttled, reject non-essential tasks
        if (thermalThrottled && priority >= MKTokenPriority::BACKGROUND) {
            std::cout << "[TOKEN SCHED] Rejected \"" << name << "\" — thermal throttle active.\n";
            return;
        }
        
        // If on battery, reject IDLE tasks entirely
        if (onBattery && priority == MKTokenPriority::IDLE) {
            return;
        }
        
        MKScheduledTask task;
        task.taskId = totalTasksRun;
        task.name = name;
        task.subsystem = subsystem;
        task.priority = priority;
        task.tokenId = grantToken(subsystem, priority, estimatedSlices);
        task.estimatedSlices = estimatedSlices;
        task.yielded = false;
        task.preempted = false;
        task.execute = fn;
        
        taskQueue.push(task);
    }
    
    // Execute the next highest priority task
    void runNext() {
        if (taskQueue.empty()) return;
        
        MKScheduledTask task = taskQueue.top();
        taskQueue.pop();
        totalTasksRun++;
        
        std::cout << "[TOKEN SCHED] Executing: \"" << task.name 
                  << "\" [" << task.subsystem << "] Priority=" << (int)task.priority << "\n";
        
        if (task.execute) task.execute();
    }
    
    // Run all pending tasks in priority order
    void drain() {
        while (!taskQueue.empty()) runNext();
    }
    
    // Update thermal state from monitor
    void setThermalState(float scale, bool throttled) {
        performanceScale = scale;
        thermalThrottled = throttled;
    }
    
    void setPowerState(bool battery) { onBattery = battery; }
    bool isAIBurstActive() const { return aiBurstActive; }
    int pendingTasks() const { return taskQueue.size(); }
    int deferredCount() const { return deferredTasks.size(); }
    
    void printStats() const {
        std::cout << "\n--- [TOKEN SCHEDULER STATS] ---\n";
        std::cout << "  Tasks Run: " << totalTasksRun << " | Pending: " << taskQueue.size() << "\n";
        std::cout << "  Deferred: " << deferredTasks.size() << " | Yields: " << totalYields << "\n";
        std::cout << "  Tokens Granted: " << totalTokensGranted << " | AI Tokens: " << aiTokensGranted << "\n";
        std::cout << "  AI Burst: " << (aiBurstActive ? "ACTIVE" : "inactive") << "\n";
        std::cout << "  Thermal: " << (thermalThrottled ? "THROTTLED" : "normal")
                  << " | Scale: " << (performanceScale * 100) << "%\n";
        std::cout << "  Power: " << (onBattery ? "BATTERY" : "AC") << "\n";
        std::cout << "-------------------------------\n";
    }
};

#endif // MK_TOKEN_SCHEDULER_CPP
