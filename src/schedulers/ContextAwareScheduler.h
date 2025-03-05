#ifndef CONTEXT_AWARE_SCHEDULER_H
#define CONTEXT_AWARE_SCHEDULER_H

#include "../models/FogNode.h"
#include "../models/Process.h"
#include "BaseScheduler.h"
#include <vector>
#include <map>
#include <memory>
#include <limits>
#include <algorithm>

class ContextAwareScheduler : public BaseScheduler {
private:
    std::vector<FogNode> fogNodes;
    std::map<int, int> processToNodeMap;

    // Override the inherited getNextProcess from BaseScheduler
    std::shared_ptr<Process> getNextProcess() override;
    
    // Node assignment and partitioning methods
    int assignToFogNode(std::shared_ptr<Process> process);
    bool partitionProcess(std::shared_ptr<Process> process);

    // Helper methods for scoring and evaluation
    double calculateLocationScore(const std::shared_ptr<Process>& process, const FogNode& node);
    double calculateLoadBalanceScore(const FogNode& node);

public:
    // Constructor
    ContextAwareScheduler(const std::vector<FogNode>& nodes);
    
    // Override methods from BaseScheduler
    void addProcess(std::shared_ptr<Process> process) override;
    void schedule() override;
    
    // Additional methods
    void printSchedulingState() const;

    // Virtual destructor
    virtual ~ContextAwareScheduler() = default;
};

#endif // CONTEXT_AWARE_SCHEDULER_H