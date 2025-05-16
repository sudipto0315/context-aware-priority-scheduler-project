#ifndef SJF_SCHEDULER_H
#define SJF_SCHEDULER_H

#include "../models/FogNode.h"
#include "../models/Process.h"
#include "BaseScheduler.h"
#include <queue>
#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>

class SJFScheduler : public BaseScheduler {
private:
    // Data structures
    std::vector<FogNode> fogNodes; // List of fog nodes
    std::priority_queue<std::shared_ptr<Process>, std::vector<std::shared_ptr<Process>>, std::function<bool(std::shared_ptr<Process>, std::shared_ptr<Process>)>> processQueue; // SJF queue based on burst time
    std::unordered_map<int, double> nodeUsedBandwidth; // Track bandwidth usage per node
    std::unordered_map<int, FogNode*> nodeMap; // Map node ID to node pointer
    std::vector<std::tuple<std::shared_ptr<Process>, std::vector<int>, bool, double, double>> allProcesses; // Track all processes
    std::vector<std::pair<std::shared_ptr<Process>, std::vector<int>>> scheduledProcesses; // Track scheduled processes
    std::unordered_map<int, std::vector<int>> processToNodeMap; // Map process ID to assigned node IDs

    // Helper methods
    bool canNodeHandleProcess(const FogNode& node, const Process& process) const;
    bool canResourcesFit(const FogNode& node, const Resource& resources, const Process& process) const;
    int assignToFogNode(std::shared_ptr<Process> process);
    void updateNodeIndices(int nodeId);

    // Metrics counters
    static int resourceCheckCount;
    static int assignmentAttemptCount;

public:
    // Constructor
    SJFScheduler(const std::vector<FogNode>& nodes);

    // Override base class methods
    void addProcess(std::shared_ptr<Process> process) override;
    void schedule() override;
    std::shared_ptr<Process> getNextProcess() override;
    void printSchedulingSummary() const override;
    void printSchedulingMetrics() const override;
};

#endif // SJF_SCHEDULER_H