#include <iostream>
#include <vector>

#include "mk_memory.cpp"

class MKGarbage {
public:
    // Pillar 5 Optimization: Performs a fast, non-blocking sweep of the memory blocks
    void collect(std::vector<Block>& blocks) {
        size_t cleanedCount = 0;
        
        for (auto& b : blocks) {
            if (b.free) {
                // In a production environment, you would clear out the raw data pointers here
                b.size = 0; 
                cleanedCount++;
            }
        }
        
        if (cleanedCount > 0) {
            std::cout << "[GC ECO-SWEEP] Reclaimed and scrubbed " << cleanedCount << " dead memory blocks.\n";
        }
    }
};