#ifndef MK_MEMORY_CPP
#define MK_MEMORY_CPP

#include <iostream>
#include <vector>
#include <string>

// The unified structure definition shared across the subsystem
struct Block {
    int id;
    size_t size;
    bool free;
};

class MKMemory {
private:
    size_t totalRAM;
    size_t usedRAM;

public:
    // Exposed publicly so MKGarbage and testing classes can read blocks safely
    std::vector<Block> blocks;

    MKMemory(size_t ram) : totalRAM(ram), usedRAM(0) {}

    int allocate(size_t size) {
        if (usedRAM + size > totalRAM) {
            std::cout << "[MEMORY] Not enough RAM, consider swap.\n";
            return -1;
        }
        Block b{(int)blocks.size(), size, false};
        blocks.push_back(b);
        usedRAM += size;
        std::cout << "[MEMORY] Allocated block " << b.id << " (" << size << " bytes)\n";
        return b.id;
    }

    void free(int id) {
        if (id >= 0 && id < (int)blocks.size() && !blocks[id].free) {
            blocks[id].free = true;
            usedRAM -= blocks[id].size;
            std::cout << "[MEMORY] Freed block " << id << '\n';
        }
    }

    void status() {
        std::cout << "[MEMORY] Total RAM: " << totalRAM 
                  << " | Used: " << usedRAM 
                  << " | Free: " << (totalRAM - usedRAM) << '\n';
    }
};

#endif // MK_MEMORY_CPP