#include <iostream>
#include <fstream>
#include <string>
#include <vector>

class MKDatasetLoader {
public:
    std::vector<std::string> loadTXT(const std::string& filename) {
        std::vector<std::string> data;
        std::ifstream in(filename);
        
        // Safety Upgrade: Explicitly verify if the file exists on your disk
        if (!in.is_open()) {
            std::cerr << "[DATASET ERROR] Failed to open path: " << filename 
                      << " (Check if file exists)" << std::endl;
            return data; // Returns empty vector safely
        }

        std::string line;
        while (std::getline(in, line)) {
            data.push_back(line);
        }
        in.close();
        
        std::cout << "[DATASET] Loaded " << data.size() << " entries from " << filename << std::endl;
        return data;
    }
};
