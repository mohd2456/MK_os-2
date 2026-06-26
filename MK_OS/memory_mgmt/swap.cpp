#ifndef MK_SWAP_CPP
#define MK_SWAP_CPP

#include <iostream>
#include <map>

class MKSwap {
private:
    std::map<int, size_t> swapSpace;

public:
    // Pillar 2: Simulates evicting "cold" memory chunks out of active RAM 
    // down into disk storage to preserve precious system cache space.
    void swapOut(int id, size_t size) {
        swapSpace[id] = size;
        std::cout << "[SWAP] Task " << id << " moved to swap (" << size << " bytes)\n";
    }

    // Restores a page frame when a task comes back online
    void swapIn(int id) {
        if (swapSpace.find(id) != swapSpace.end()) {
            std::cout << "[SWAP] Task " << id << " restored from swap.\n";
            swapSpace.erase(id);
        }
    }
};

#endif // MK_SWAP_CPP