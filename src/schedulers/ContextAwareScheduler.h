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
#include <iostream>

class ContextAwareScheduler : public BaseScheduler {
private:
    std::vector<FogNode> fogNodes;
    std::vector<FogNode> sortedNodes;  // Pre-sorted nodes for performance
    std::vector<std::shared_ptr<Process>> retryQueue;  // Queue for failed assignments
    std::map<int, std::vector<int>> processToNodeMap;  // Maps process ID to list of fog node IDs

    // Override the inherited getNextProcess from BaseScheduler
    std::shared_ptr<Process> getNextProcess() override;
    
    // Node assignment and partitioning methods
    int assignToFogNode(std::shared_ptr<Process> process);
    bool partitionProcess(std::shared_ptr<Process> process);

    // Helper methods for scoring and evaluation
    double calculateLocationScore(const std::shared_ptr<Process>& process, const FogNode& node);
    double calculateLoadBalanceScore(const FogNode& node);
    double calculateProcessScore(const std::shared_ptr<Process>& process);
    double calculateNodeScore(const FogNode& node, const std::shared_ptr<Process>& process);
    double calculateNewLoad(const FogNode& node, const Resource& resources);
    bool canResourcesFit(const FogNode& node, const Resource& resources, const Process& process);
    bool canNodeHandleProcess(const FogNode& node, const Process& process);

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