#include <iostream>
#include <fstream>
#include <string>

class MKFileIO {
public:
    void writeFile(const std::string& filename, const std::string& data) {
        std::ofstream out(filename, std::ios::binary);
        if (!out.is_open()) {
            std::cerr << "[FILE ERROR] Cannot open path for writing: " << filename << '\n';
            return;
        }
        out << data;
        out.close();
        std::cout << "[FILE] Written to " << filename << '\n';
    }

    std::string readFile(const std::string& filename) {
        std::ifstream in(filename, std::ios::binary);
        if (!in.is_open()) {
            std::cerr << "[FILE ERROR] Cannot open path for reading: " << filename << '\n';
            return "";
        }
        std::string data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        in.close();
        std::cout << "[FILE] Read from " << filename << '\n';
        return data;
    }

    // Pillar 2 Simulation: Simulates loading a "hot layer" weight block 
    // from storage directly into a fixed RAM cache window rather than loading the whole file.
    void pageModelLayer(const std::string& weightFilename, size_t offset, size_t length, char* activeRamCache) {
        std::ifstream in(weightFilename, std::ios::binary);
        if (!in.is_open() || !activeRamCache) {
            std::cerr << "[PAGER ERROR] Model pagination fault for file: " << weightFilename << '\n';
            return;
        }

        // Seek directly to the active operational layer block
        in.seekg(offset);
        in.read(activeRamCache, length);
        in.close();

        std::cout << "[NEXUS PAGER] Paged " << length << " bytes from layer offset " 
                  << offset << " into active execution cache.\n";
    }
};