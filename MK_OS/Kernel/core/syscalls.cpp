#ifndef MK_SYSCALLS_CPP
#define MK_SYSCALLS_CPP

#include <iostream>
#include <string>
#include <map>
#include <functional>

// MK Syscall return codes
enum class MKSysResult { OK = 0, ERR_INVALID = -1, ERR_PERM = -2, ERR_NOTFOUND = -3 };

class MKSyscalls {
private:
    // Syscall dispatch table — maps syscall ID to handler name
    std::map<int, std::string> syscallTable = {
        {0,  "SYS_LOG"},
        {1,  "SYS_HALT"},
        {2,  "SYS_ALLOC"},
        {3,  "SYS_FREE"},
        {4,  "SYS_SPAWN"},
        {5,  "SYS_KILL"},
        {6,  "SYS_SLEEP"},
        {7,  "SYS_WAKE"},
        {8,  "SYS_READ"},
        {9,  "SYS_WRITE"},
        {10, "SYS_YIELD"},
    };

public:
    MKSyscalls() {
        std::cout << "[KERNEL - SYSCALL] Interrupt vector table initialized. "
                  << syscallTable.size() << " syscalls registered.\n";
    }

    // Log message from userland — syscall 0
    MKSysResult log(const std::string& msg) {
        std::cout << "[SYSCALL:LOG] " << msg << "\n";
        return MKSysResult::OK;
    }

    // Halt the system — syscall 1
    MKSysResult halt() {
        std::cout << "[SYSCALL:HALT] System shutdown requested. Flushing state...\n";
        return MKSysResult::OK;
    }

    // Yield CPU from current task — syscall 10
    MKSysResult yield() {
        std::cout << "[SYSCALL:YIELD] Task voluntarily yielding CPU slice.\n";
        return MKSysResult::OK;
    }

    // Generic dispatch by syscall ID
    MKSysResult dispatch(int id, const std::string& arg = "") {
        if(syscallTable.find(id) == syscallTable.end()) {
            std::cout << "[SYSCALL] ERR: Unknown syscall ID=" << id << "\n";
            return MKSysResult::ERR_INVALID;
        }
        std::cout << "[SYSCALL] Dispatching " << syscallTable[id];
        if(!arg.empty()) std::cout << " | arg=\"" << arg << "\"";
        std::cout << "\n";
        return MKSysResult::OK;
    }

    void printTable() const {
        std::cout << "\n--- [SYSCALL TABLE] ---\n";
        for(const auto& s : syscallTable)
            std::cout << "  [" << s.first << "] " << s.second << "\n";
        std::cout << "-----------------------\n";
    }
};

#endif // MK_SYSCALLS_CPP