#include <iostream>

// Include the base data structures first
#include "mk_memory.cpp"
#include "swap.cpp"
#include "garbage.cpp"

// Include allocator last since it interfaces with them
#include "allocator.cpp"

int main() {
    std::cout << "[MEMORY TEST] Initializing Subsystem Test Matrix...\n";
    
    MKMemory memory(1024); // 1KB RAM sandbox for demo
    MKAllocator allocator;
    MKSwap swap;
    MKGarbage gc;

    // 1. Allocation Cycle
    int task1 = allocator.allocateTask(memory, 200);
    int task2 = allocator.allocateTask(memory, 500);
    memory.status();

    // 2. Freeing Cycle
    allocator.freeTask(memory, task1);
    memory.status();

    // 3. Swap Paging Test (Pillar 2)
    swap.swapOut(task2, 500);
    swap.swapIn(task2);

    // 4. Garbage Clean Sweep
    gc.collect(memory.blocks);

    std::cout << "[MEMORY TEST] All cycles executed cleanly.\n";
    return 0;
}