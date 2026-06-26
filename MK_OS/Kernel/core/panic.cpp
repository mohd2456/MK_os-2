#ifndef MK_PANIC_CPP
#define MK_PANIC_CPP

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <ctime>

enum class MKPanicCode {
    GENERAL        = 0,
    NULL_DEREF     = 1,
    STACK_OVERFLOW = 2,
    INVALID_OPCODE = 3,
    OUT_OF_MEMORY  = 4,
    DRIVER_FAULT   = 5,
    SCHED_CORRUPT  = 6,
    ASSERT_FAIL    = 7,
};

class MKPanic {
private:
    std::string codeLabel(MKPanicCode code) {
        switch(code) {
            case MKPanicCode::GENERAL:        return "GENERAL_FAULT";
            case MKPanicCode::NULL_DEREF:     return "NULL_DEREFERENCE";
            case MKPanicCode::STACK_OVERFLOW: return "STACK_OVERFLOW";
            case MKPanicCode::INVALID_OPCODE: return "INVALID_OPCODE";
            case MKPanicCode::OUT_OF_MEMORY:  return "OUT_OF_MEMORY";
            case MKPanicCode::DRIVER_FAULT:   return "DRIVER_FAULT";
            case MKPanicCode::SCHED_CORRUPT:  return "SCHEDULER_CORRUPT";
            case MKPanicCode::ASSERT_FAIL:    return "ASSERTION_FAILED";
        }
        return "UNKNOWN";
    }

    void dumpSnapshot(const std::string& reason, MKPanicCode code) {
        std::time_t now = std::time(nullptr);
        std::cerr << "\n";
        std::cerr << "##################################################\n";
        std::cerr << "#              MK KERNEL PANIC                   #\n";
        std::cerr << "##################################################\n";
        std::cerr << "  Code    : [" << (int)code << "] " << codeLabel(code) << "\n";
        std::cerr << "  Reason  : " << reason << "\n";
        std::cerr << "  Time    : " << std::ctime(&now);
        std::cerr << "  State   : Halting all execution rings.\n";
        std::cerr << "  Action  : Protect memory. Await cold reboot.\n";
        std::cerr << "##################################################\n\n";
    }

public:
    // Hard panic — system halts permanently
    [[noreturn]] void panic(const std::string& reason, 
                            MKPanicCode code = MKPanicCode::GENERAL) {
        dumpSnapshot(reason, code);
        while(true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

    // Soft assert — panics only if condition is false
    void assert_mk(bool condition, const std::string& msg,
                   MKPanicCode code = MKPanicCode::ASSERT_FAIL) {
        if(!condition) {
            panic("Assertion failed: " + msg, code);
        }
    }
};

#endif // MK_PANIC_CPP