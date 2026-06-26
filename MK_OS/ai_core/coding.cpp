#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>

class MKCoding {
public:
    void writeCode(const std::string& filename, const std::string& code) {
        std::ofstream out(filename);
        if (out.is_open()) {
            out << code;
            out.close();
        } else {
            std::cerr << "[ERROR] Failed to write code to " << filename << std::endl;
        }
    }

    std::string compileAndRun(const std::string& filename) {
        // -O2 optimizes for speed on your CPU
        // && rm mk_program cleans up the temporary file immediately after running
        std::string command = "g++ -O2 " + filename + " -o mk_program && ./mk_program && rm mk_program";
        
        int result = system(command.c_str());
        return (result == 0) ? "Program executed successfully" : "Compilation/Execution error";
    }
};