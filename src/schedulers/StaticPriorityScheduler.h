#ifndef STATIC_PRIORITY_SCHEDULER_H
#define STATIC_PRIORITY_SCHEDULER_H

#include "BaseScheduler.h"
#include "../models/Process.h"
#include <vector>
#include <queue>

class StaticPriorityScheduler : public BaseScheduler {
public:
    StaticPriorityScheduler();

    void schedule() override;
    std::shared_ptr<Process> getNextProcess() override; // Added override for the pure virtual function
};

#endif // STATIC_PRIORITY_SCHEDULER_H