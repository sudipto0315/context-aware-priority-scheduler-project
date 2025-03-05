#ifndef CONTEXT_AWARE_SCHEDULER_H
#define CONTEXT_AWARE_SCHEDULER_H

#include "../models/FogNode.h"
#include "../models/Process.h"
#include "BaseScheduler.h"
#include <vector>
#include <map>
#include <memory>

class ContextAwareScheduler : public BaseScheduler {
private:
    std::vector<FogNode> fogNodes;
    std::vector<std::shared_ptr<Process>> processQueue;
    std::map<int, int> processToNodeMap;

    std::shared_ptr<Process> getNextProcess();
    int assignToFogNode(std::shared_ptr<Process> process);
    bool partitionProcess(std::shared_ptr<Process> process);  // New method

public:
    ContextAwareScheduler(const std::vector<FogNode>& nodes);
    void addProcess(std::shared_ptr<Process> process) override;
    void schedule() override;
    void printSchedulingState() const;
};

#endif