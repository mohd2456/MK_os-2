#include "mk_memory.cpp"

class MKAllocator {
public:
    // Performance Upgrade: Making these inline eliminates function-call overhead,
    // allowing raw pointer/task allocations to execute at pure hardware speeds.
    inline int allocateTask(MKMemory& mem, size_t size) {
        return mem.allocate(size);
    }

    inline void freeTask(MKMemory& mem, int id) {
        mem.free(id);
    }
};