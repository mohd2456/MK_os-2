#include <iostream>
#include <cstdlib>
#include <string>

class MKExecution {
public:
    std::string run(const std::string& task) {
        if (task.empty()) {
            return "Error: Empty task command";
        }
        
        std::cout << "[EXECUTION] Running system task: " << task << std::endl;
        
        // Executes the command directly on the Mac OS X subsystem
        int result = system(task.c_str());
        
        return (result == 0) ? "Task executed successfully" : "Execution error";
    }
};
