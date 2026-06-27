#ifndef MK_DAEMON_CPP
#define MK_DAEMON_CPP

#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <atomic>
#include <vector>
#include <functional>
#include <ctime>

// ===================================================================================
// MK DAEMON — 24/7 Server Loop
// ===================================================================================
// MK runs FOREVER on the Mac. This is the master daemon that:
//   - Keeps MK alive 24/7 (auto-restart on crash)
//   - Thermal-aware main loop (sleeps more when hot)
//   - Schedules background tasks during cool/idle periods
//   - Handles graceful shutdown signals
//   - Logs uptime and health metrics
//   - Coordinates between all subsystems
// ===================================================================================

enum class MKDaemonState {
    STARTING,
    RUNNING,
    THROTTLED,      // Reduced activity due to heat
    COOLDOWN,       // Paused, waiting for temp drop
    MAINTENANCE,    // Running background tasks
    SHUTTING_DOWN
};

struct MKScheduledJob {
    std::string name;
    std::function<void()> execute;
    int intervalSeconds;
    int lastRun;        // Seconds since daemon start
    bool requiresCool;  // Only run when temp is low
    bool enabled;
};

class MKDaemon {
private:
    MKDaemonState state;
    std::atomic<bool> running;
    std::vector<MKScheduledJob> jobs;
    std::chrono::steady_clock::time_point startTime;
    int loopCount;
    int crashCount;
    float currentTemp;           // Fed from thermal governor
    float workloadCap;           // Fed from thermal governor
    std::string logFile;

public:
    MKDaemon() : state(MKDaemonState::STARTING), running(true),
                 loopCount(0), crashCount(0), currentTemp(50.0f),
                 workloadCap(1.0f), logFile("mk_daemon.log") {
        startTime = std::chrono::steady_clock::now();
        std::cout << "[DAEMON] MK 24/7 server daemon initializing...\n";
    }

    // Register a recurring background job
    void addJob(const std::string& name, int intervalSec,
                std::function<void()> fn, bool needsCool = false) {
        MKScheduledJob job;
        job.name = name;
        job.execute = fn;
        job.intervalSeconds = intervalSec;
        job.lastRun = 0;
        job.requiresCool = needsCool;
        job.enabled = true;
        jobs.push_back(job);
        std::cout << "[DAEMON] Job registered: \"" << name 
                  << "\" every " << intervalSec << "s"
                  << (needsCool ? " (cool only)" : "") << "\n";
    }

    // Update thermal info (called by thermal governor)
    void updateThermal(float temp, float cap) {
        currentTemp = temp;
        workloadCap = cap;
        
        if (cap < 0.1f) state = MKDaemonState::COOLDOWN;
        else if (cap < 0.5f) state = MKDaemonState::THROTTLED;
        else state = MKDaemonState::RUNNING;
    }

    // Get uptime in seconds
    long uptimeSeconds() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();
    }

    // Run one tick of the daemon loop
    void tick() {
        loopCount++;
        long uptime = uptimeSeconds();
        
        // Check scheduled jobs
        for (auto& job : jobs) {
            if (!job.enabled) continue;
            if (uptime - job.lastRun < job.intervalSeconds) continue;
            if (job.requiresCool && currentTemp > 60.0f) continue;
            if (state == MKDaemonState::COOLDOWN && job.requiresCool) continue;
            
            job.execute();
            job.lastRun = uptime;
        }
    }

    // Calculate how long to sleep between ticks (thermal-aware)
    int getSleepMs() const {
        switch (state) {
            case MKDaemonState::RUNNING:      return 1000;   // 1 second
            case MKDaemonState::THROTTLED:    return 3000;   // 3 seconds
            case MKDaemonState::COOLDOWN:     return 10000;  // 10 seconds
            case MKDaemonState::MAINTENANCE:  return 2000;
            default: return 1000;
        }
    }

    // Graceful shutdown
    void shutdown() {
        state = MKDaemonState::SHUTTING_DOWN;
        running = false;
        std::cout << "[DAEMON] Shutdown signal received. Uptime: " 
                  << uptimeSeconds() << " seconds.\n";
    }

    bool isRunning() const { return running.load(); }
    MKDaemonState getState() const { return state; }
    int getLoopCount() const { return loopCount; }

    std::string stateLabel() const {
        switch (state) {
            case MKDaemonState::STARTING:      return "STARTING";
            case MKDaemonState::RUNNING:       return "RUNNING";
            case MKDaemonState::THROTTLED:     return "THROTTLED";
            case MKDaemonState::COOLDOWN:      return "COOLDOWN";
            case MKDaemonState::MAINTENANCE:   return "MAINTENANCE";
            case MKDaemonState::SHUTTING_DOWN: return "SHUTTING DOWN";
        }
        return "UNKNOWN";
    }

    void printStatus() const {
        std::cout << "\n━━━ [MK DAEMON] ━━━\n";
        std::cout << "  State: " << stateLabel() << "\n";
        std::cout << "  Uptime: " << (uptimeSeconds() / 3600) << "h " 
                  << ((uptimeSeconds() % 3600) / 60) << "m\n";
        std::cout << "  Loops: " << loopCount << "\n";
        std::cout << "  Temp: " << currentTemp << "°C | Cap: " 
                  << (workloadCap * 100) << "%\n";
        std::cout << "  Jobs: " << jobs.size() << " registered\n";
        std::cout << "  Sleep: " << getSleepMs() << "ms per tick\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━\n";
    }
};

#endif // MK_DAEMON_CPP
