#ifndef MK_PC_HELPER_CPP
#define MK_PC_HELPER_CPP

#include <iostream>
#include <string>
#include <cstdlib> // Needed for std::system execution calls

class MKPCHelper {
public:
    MKPCHelper() {
        std::cout << "[PLUGIN - PC HELPER] Initializing platform automation tools...\n";
    }

    // Executes a safe shell script command directly into the native runtime environment
    void runCommand(const std::string& command) {
        std::cout << "[PC HELPER] Executing environment terminal script: " << command << "\n";
        // Dispatches directly to your local computer's processor shell loop
        std::system(command.c_str());
    }

    // Automatically spins up local viewer utilities to inspect configuration files
    void openFile(const std::string& filename) {
        std::cout << "[PC HELPER] Locating and spawning viewer for target payload: " << filename << "\n";
        
        // Multi-platform file opening utility helper logic
        std::string command;
#if defined(_WIN32)
        command = "start " + filename;
#elif defined(__APPLE__)
        command = "open " + filename;
#else
        command = "xdg-open " + filename;
#endif
        std::system(command.c_str());
    }
};

#endif // MK_PC_HELPER_CPP