#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <string>

class MKDatasetParser {
public:
    std::map<std::string, std::string> parseCSV(const std::string& filename) {
        std::map<std::string, std::string> data;
        std::ifstream in(filename);
        
        if (!in.is_open()) {
            std::cerr << "[PARSER ERROR] Cannot open CSV file: " << filename << std::endl;
            return data;
        }

        std::string line;
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            std::stringstream ss(line);
            std::string key, value;
            if (std::getline(ss, key, ',') && std::getline(ss, value, ',')) {
                data[key] = value;
            }
        }
        in.close();
        std::cout << "[PARSER] Successfully parsed " << data.size() << " entries from CSV." << std::endl;
        return data;
    }

    std::map<std::string, std::string> parseJSON(const std::string& filename) {
        std::map<std::string, std::string> data;
        std::ifstream in(filename);
        
        if (!in.is_open()) {
            std::cerr << "[PARSER ERROR] Cannot open JSON file: " << filename << std::endl;
            return data;
        }

        // Lightweight JSON scanner looking for raw "key": "value" patterns
        std::string line;
        while (std::getline(in, line)) {
            size_t firstQuote = line.find('"');
            if (firstQuote == std::string::npos) continue;

            size_t secondQuote = line.find('"', firstQuote + 1);
            size_t colon = line.find(':', secondQuote + 1);
            size_t thirdQuote = line.find('"', colon + 1);
            size_t fourthQuote = line.find('"', thirdQuote + 1);

            if (secondQuote != std::string::npos && colon != std::string::npos && 
                thirdQuote != std::string::npos && fourthQuote != std::string::npos) {
                
                std::string key = line.substr(firstQuote + 1, secondQuote - firstQuote - 1);
                std::string value = line.substr(thirdQuote + 1, fourthQuote - thirdQuote - 1);
                data[key] = value;
            }
        }
        in.close();
        std::cout << "[PARSER] Successfully parsed " << data.size() << " entries from JSON." << std::endl;
        return data;
    }
};