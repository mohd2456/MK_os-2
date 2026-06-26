#ifndef MK_CONSOLE_CPP
#define MK_CONSOLE_CPP

#include <iostream>
#include <string>
#include <sstream>

enum class MKLogLevel { INFO, WARN, ERROR, DEBUG };

class MKConsoleDriver {
private:
    bool colorEnabled = true;
    int lineCount = 0;

    std::string levelTag(MKLogLevel lvl) const {
        switch(lvl) {
            case MKLogLevel::INFO:  return "[INFO] ";
            case MKLogLevel::WARN:  return "[WARN] ";
            case MKLogLevel::ERROR: return "[ERR]  ";
            case MKLogLevel::DEBUG: return "[DBG]  ";
        }
        return "[?]    ";
    }

public:
    MKConsoleDriver() {
        std::cout << "[DRIVER] Console pipeline attached to output stream.\n";
    }

    // Write a plain message
    void write(const std::string& msg) {
        lineCount++;
        std::cout << "[CONSOLE] " << msg << "\n";
    }

    // Write with log level
    void log(const std::string& msg, MKLogLevel lvl = MKLogLevel::INFO) {
        lineCount++;
        std::cout << "[CONSOLE]" << levelTag(lvl) << msg << "\n";
    }

    // Read a line of input from the user
    std::string read(const std::string& prompt = "") {
        if(!prompt.empty())
            std::cout << prompt << " ";
        std::string line;
        if(std::getline(std::cin, line))
            return line;
        return "";
    }

    // Print a divider line
    void divider(char c = '-', int len = 50) {
        std::cout << std::string(len, c) << "\n";
    }

    // Print MK banner
    void banner() {
        divider('=');
        std::cout << "   MK HYBRID OS — KERNEL CONSOLE\n";
        divider('=');
    }

    int lines() const { return lineCount; }
};

#endif // MK_CONSOLE_CPP