#ifndef MK_TASK_CPP
#define MK_TASK_CPP

#include <string>

// Pillar 3 Scheduling: The atomic processing token for the OS engine
struct Task {
    std::string name;
    int priority; // 1 = fast (immediate execution), 2 = deep (resource intensive)
};

#endif // MK_TASK_CPP