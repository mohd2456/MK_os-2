#include <iostream>
#include <unordered_map>
#include <fstream>
#include <string>

class MKMemory {
private:
    std::unordered_map<std::string, std::string> knowledge;
public:
    void store(const std::string& key, const std::string& value) {
        knowledge[key] = value;
    }

    std::string search(const std::string& key) {
        return (knowledge.find(key) != knowledge.end()) ? knowledge[key] : "Not found";
    }

    void saveToFile(const std::string& filename) {
        std::ofstream out(filename);
        if (out.is_open()) {
            for (const auto& pair : knowledge) {
                out << pair.first << "=" << pair.second << "\n";
            }
            out.close();
        }
    }

    void loadFromFile(const std::string& filename) {
        std::ifstream in(filename);
        std::string line;
        while (std::getline(in, line)) {
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                knowledge[key] = value;
            }
        }
        in.close();
    }

    // Added to prevent compilation crashes in main.cpp
    void status() {
        std::cout << "[MEMORY STATUS] Active short-term cache slots filled: " 
                  << knowledge.size() << " entries." << std::endl;
    }
};