#ifndef NONPREEMPTIVE_SCHEDULER_H
#define NONPREEMPTIVE_SCHEDULER_H

#include "BaseScheduler.h"  
#include "../models/Process.h"
#include "../models/FogNode.h"
#include <vector>
#include <queue>

class NonPreemptiveScheduler : public BaseScheduler {
private:
    std::vector<FogNode> fogNodes; // Store available Fog Nodes

public:
    explicit NonPreemptiveScheduler(const std::vector<FogNode>& nodes);

    void schedule() override;
    std::shared_ptr<Process> getNextProcess() override; 
};

#endif // NONPREEMPTIVE_SCHEDULER_H