#ifndef MK_TIMER_CPP
#define MK_TIMER_CPP

#include <iostream>
#include <functional>
#include <vector>
#include <chrono>

struct MKTimerEvent {
    int intervalTicks;
    int countdown;
    std::string label;
    std::function<void()> callback;
};

class MKTimerDriver {
private:
    long long totalTicks = 0;
    int freqHz = 100; // Simulated 100Hz timer
    std::vector<MKTimerEvent> events;
    std::chrono::steady_clock::time_point bootTime;

public:
    MKTimerDriver() {
        std::cout << "[DRIVER] Timer bound to IRQ0 at " << freqHz << "Hz.\n";
        bootTime = std::chrono::steady_clock::now();
    }

    void init() {
        std::cout << "[DRV:TIMER] Timer initialized. Frequency=" << freqHz << "Hz\n";
    }

    // Fire one timer tick — advances uptime and triggers events
    void tick() {
        totalTicks++;

        // Fire any registered events
        for(auto& e : events) {
            e.countdown--;
            if(e.countdown <= 0) {
                std::cout << "[DRV:TIMER] Event fired: \"" << e.label << "\"\n";
                if(e.callback) e.callback();
                e.countdown = e.intervalTicks; // Reset
            }
        }
    }

    // Register a recurring timer event
    void registerEvent(const std::string& label, int intervalTicks,
                       std::function<void()> cb = nullptr) {
        events.push_back({intervalTicks, intervalTicks, label, cb});
        std::cout << "[DRV:TIMER] Event registered: \"" << label
                  << "\" every " << intervalTicks << " ticks.\n";
    }

    long long uptime() const { return totalTicks; }

    // Real wall-clock uptime in milliseconds
    long long wallMs() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>
               (now - bootTime).count();
    }

    void stats() const {
        std::cout << "[DRV:TIMER] Ticks=" << totalTicks
                  << " | Wall=" << wallMs() << "ms"
                  << " | Events=" << events.size() << "\n";
    }
};

#endif // MK_TIMER_CPP