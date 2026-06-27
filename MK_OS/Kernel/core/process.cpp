#ifndef MK_PROCESS_CPP
#define MK_PROCESS_CPP

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

enum class MKProcessState { RUNNING, SLEEPING, STOPPED, ZOMBIE };

struct MKProcess {
    int pid;
    std::string name;
    MKProcessState state;
    int priority;        // 0 = highest (kernel), 9 = lowest (background)
    int ticksAlive;
};

class MKProcessTable {
private:
    std::vector<MKProcess> procs;
    int nextPid = 1;

    std::string stateLabel(MKProcessState s) const {
        switch(s) {
            case MKProcessState::RUNNING:  return "RUNNING";
            case MKProcessState::SLEEPING: return "SLEEPING";
            case MKProcessState::STOPPED:  return "STOPPED";
            case MKProcessState::ZOMBIE:   return "ZOMBIE";
        }
        return "UNKNOWN";
    }

public:
    MKProcessTable() {
        std::cout << "[KERNEL - PROCESS] Process allocation table ready.\n";
    }

    int create(const std::string& name, int priority = 5) {
        int pid = nextPid++;
        procs.push_back({pid, name, MKProcessState::RUNNING, priority, 0});
        std::cout << "[KERNEL] Process created: \"" << name 
                  << "\" | PID=" << pid << " | Priority=" << priority << "\n";
        return pid;
    }

    // Kill a process by PID — marks as ZOMBIE for cleanup
    bool kill(int pid) {
        for(auto& p : procs) {
            if(p.pid == pid && p.state != MKProcessState::ZOMBIE) {
                p.state = MKProcessState::ZOMBIE;
                std::cout << "[KERNEL] Process PID=" << pid << " (\"" << p.name << "\") terminated.\n";
                return true;
            }
        }
        std::cout << "[KERNEL] Kill failed: PID=" << pid << " not found or already dead.\n";
        return false;
    }

    // Sleep a process
    bool sleep(int pid) {
        for(auto& p : procs) {
            if(p.pid == pid && p.state == MKProcessState::RUNNING) {
                p.state = MKProcessState::SLEEPING;
                std::cout << "[KERNEL] Process PID=" << pid << " sleeping.\n";
                return true;
            }
        }
        return false;
    }

    // Wake a sleeping process
    bool wake(int pid) {
        for(auto& p : procs) {
            if(p.pid == pid && p.state == MKProcessState::SLEEPING) {
                p.state = MKProcessState::RUNNING;
                std::cout << "[KERNEL] Process PID=" << pid << " woken.\n";
                return true;
            }
        }
        return false;
    }

    // Tick all running processes — advances their lifetime counter
    void tick() {
        for(auto& p : procs) {
            if(p.state == MKProcessState::RUNNING) {
                p.ticksAlive++;
            }
        }
    }

    // Clean up zombie processes from the table
    void reap() {
        int before = procs.size();
        procs.erase(std::remove_if(procs.begin(), procs.end(),
            [](const MKProcess& p){ return p.state == MKProcessState::ZOMBIE; }),
            procs.end());
        int reaped = before - procs.size();
        if(reaped > 0)
            std::cout << "[KERNEL] Reaped " << reaped << " zombie process(es).\n";
    }

    // Find PID by name
    int findPid(const std::string& name) const {
        for(const auto& p : procs)
            if(p.name == name) return p.pid;
        return -1;
    }

    int count() const { return procs.size(); }

    void list() const {
        std::cout << "\n--- [KERNEL PROCESS TABLE] ---\n";
        if(procs.empty()) {
            std::cout << "  [EMPTY] No active processes.\n";
            return;
        }
        for(const auto& p : procs) {
            std::cout << "  PID=" << p.pid
                      << " | \"" << p.name << "\""
                      << " | State=" << stateLabel(p.state)
                      << " | Priority=" << p.priority
                      << " | Ticks=" << p.ticksAlive << "\n";
        }
        std::cout << "------------------------------\n";
    }
};

#endif // MK_PROCESS_CPP