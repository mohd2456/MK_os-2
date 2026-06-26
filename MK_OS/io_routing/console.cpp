#include <iostream>
#include <string>

class MKConsoleIO {
public:
    MKConsoleIO() {
        // Performance Upgrade: Decouples C++ streams from C stdio functions.
        // This speeds up terminal input/output rendering drastically on older CPU cores.
        std::ios_base::sync_with_stdio(false);
        std::cin.tie(NULL);
    }

    std::string readInput() {
        std::string input;
        std::cout << "[CONSOLE] > " << std::flush; // Ensures the prompt appears immediately
        std::getline(std::cin, input);
        return input;
    }

    void writeOutput(const std::string& output) {
        // Using '\n' instead of std::endl avoids forcing an expensive hardware disk/buffer flush
        std::cout << "[CONSOLE] " << output << '\n';
    }
};