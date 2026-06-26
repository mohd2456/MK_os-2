#ifndef MK_UPDATE_CPP
#define MK_UPDATE_CPP

#include <iostream>
#include <fstream>
#include <string>

class MKDatasetUpdater {
public:
    // Pillar 8 Persistence: Safely appends parsed text tokens into local data storage
    void appendToDataset(const std::string& filename, const std::string& entry) {
        std::ofstream out(filename, std::ios::app);
        if (out.is_open()) {
            out << entry << "\n";
            out.close();
            std::cout << "[UPDATE] Added new entry to " << filename << "\n";
        } else {
            std::cerr << "[UPDATE ERROR] Could not open dataset file: " << filename << "\n";
        }
    }
};

#endif // MK_UPDATE_CPP