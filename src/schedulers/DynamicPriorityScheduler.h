#ifndef DYNAMIC_PRIORITY_SCHEDULER_H
#define DYNAMIC_PRIORITY_SCHEDULER_H

#include "BaseScheduler.h"
#include "../models/Process.h"
#include "../models/FogNode.h"
#include <vector>
#include <queue>

class DynamicPriorityScheduler : public BaseScheduler {
public:
    DynamicPriorityScheduler();
    
    void schedule() override;
    std::shared_ptr<Process> getNextProcess() override; // Added override for the pure virtual function
private:
    void updatePriorities(); // Function to dynamically adjust priorities
};

#endif // DYNAMIC_PRIORITY_SCHEDULER_H