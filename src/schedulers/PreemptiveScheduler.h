#ifndef PREEMPTIVESCHEDULER_H
#define PREEMPTIVESCHEDULER_H

#include "BaseScheduler.h"
#include "../models/FogNode.h"
#include <unordered_map>
#include <vector>
#include <memory>

class PreemptiveScheduler : public BaseScheduler {
private:
    std::vector<FogNode> fogNodes;  // List of available Fog nodes
    std::unordered_map<int, int> processToNodeMap; // Maps Process ID to assigned Fog Node
    std::vector<std::shared_ptr<Process>> processQueue;

public:
    // Constructor
    explicit PreemptiveScheduler(const std::vector<FogNode>& nodes);

    // Schedule tasks based on preemptive priority-based logic
    void schedule() override;

    // Get the next process considering preemptive scheduling
    std::shared_ptr<Process> getNextProcess() override;

    // Assign a process to a Fog Node, preempting if necessary
    int assignToFogNode(std::shared_ptr<Process> process);

    // Print the current scheduling state
    void printSchedulingState() const;
};

#endif // PREEMPTIVESCHEDULER_H