#ifndef MK_QUEUE_CPP
#define MK_QUEUE_CPP

#include <queue>
#include "task.cpp"

class MKQueue {
public:
    std::queue<Task> fastQueue;
    std::queue<Task> deepQueue;
};

#endif // MK_QUEUE_CPP