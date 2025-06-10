#ifndef CONTEXT_AWARE_SCHEDULER_H
#define CONTEXT_AWARE_SCHEDULER_H

#include "../models/FogNode.h"
#include "../models/Process.h"
#include "BaseScheduler.h"
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <queue>
#include <set>

// Custom comparator for priority queue
struct ProcessArrivalComparator {
    bool operator()(const std::shared_ptr<Process>& a, const std::shared_ptr<Process>& b) const {
        if (a->getArrivalTime() != b->getArrivalTime()) {
            return a->getArrivalTime() > b->getArrivalTime(); // Min-heap on arrival time
        }
        return a->getProcessScore() < b->getProcessScore(); // Max-heap on score if arrival times are equal (tie-breaker)
    }
};

class ContextAwareScheduler : public BaseScheduler {
private:
    std::vector<FogNode> fogNodes;
    std::vector<FogNode> sortedNodes;  // Pre-sorted nodes for performance
    bool needsResorting;               // Flag to determine if sorting is needed
    
    // Priority queue for processes
    std::priority_queue<std::shared_ptr<Process>, std::vector<std::shared_ptr<Process>>, ProcessArrivalComparator> processPriorityQueue; // Process queue sorted by arrival time and score
    
    // Retry queue for failed assignments
    std::priority_queue<std::shared_ptr<Process>, std::vector<std::shared_ptr<Process>>, ProcessArrivalComparator> retryQueue;
    
    // Maps process ID to list of fog node IDs
    std::map<int, std::vector<int>> processToNodeMap;
    
    // Vector to store scheduled processes and their assigned nodes
    std::vector<std::pair<std::shared_ptr<Process>, std::vector<int>>> scheduledProcesses;

    // Vector to store all processed processes (scheduled or failed)
    std::vector<std::tuple<std::shared_ptr<Process>, std::vector<int>, bool, double, double>> allProcesses;
    
    // Multi-dimensional indices for faster resource filtering
    std::set<std::pair<double, int>> nodesByCapacity;  // {capacity, nodeId}  // Maps capacity to node IDs
    std::set<std::pair<double, int>> nodesByMemory;    // {memory, nodeId}    // Maps memory to node IDs
    std::set<std::pair<double, int>> nodesByBandwidth; // {bandwidth, nodeId} // Maps bandwidth to node IDs
    
    // Process grouping for batch processing
    std::map<int, std::vector<std::shared_ptr<Process>>> processesByGroup;
    
    // Node mapping and indexing for faster lookups
    std::map<int, FogNode*> nodeMap;                   // Maps node ID to FogNode pointer
    std::unordered_map<int, size_t> nodeIndexMap;      // Maps node ID to vector index
    std::unordered_map<int, double> nodeUsedBandwidth; // Maps node ID to used bandwidth
    
    // Cache for coordinates to avoid recalculation
    std::unordered_map<int, std::pair<double, double>> nodeCoordinates;
    
    // Caches for expensive calculations
    std::unordered_map<int, double> processScoreCache;                             // Process ID to score
    std::unordered_map<int, std::pair<double, double>> loadBalanceCache;           // Node ID to {load, score}
    
    // Use map instead of unordered_map for pair keys to avoid hash complications
    std::map<std::pair<int, int>, double> nodeScoreCache;                          // {Node ID, Process ID} to score
    std::map<std::pair<int, int>, double> locationScoreCache;                      // {Process ID, Node ID} to location score
    
    // Comparator for ordering nodes by load
    struct NodeLoadComparator {
        const std::map<int, FogNode*>& nodeMap;
        NodeLoadComparator(const std::map<int, FogNode*>& nm) : nodeMap(nm) {}
        bool operator()(int a, int b) const {
            double loadA = nodeMap.at(a)->getCurrentLoad();
            double loadB = nodeMap.at(b)->getCurrentLoad();
            return (loadA != loadB) ? loadA < loadB : a < b;
        }
    };
    NodeLoadComparator nodeComparator;
    
    // Node assignment and partitioning methods
    int assignToFogNode(std::shared_ptr<Process> process);
    bool partitionProcess(std::shared_ptr<Process> process);
    
    // Process retry queue
    void processRetryQueue();

    // Helper methods for scoring and evaluation
    double calculateLocationScore(const std::shared_ptr<Process>& process, const FogNode& node);
    double calculateLoadBalanceScore(const FogNode& node);
    double calculateUserActivityLevel(const std::string& usageHistory);
    double calculateProcessScore(const std::shared_ptr<Process>& process);
    double calculateNodeScore(const FogNode& node, const std::shared_ptr<Process>& process);
    double calculateNewLoad(const FogNode& node, const Resource& resources);
    bool canResourcesFit(const FogNode& node, const Resource& resources, const Process& process);
    bool canNodeHandleProcess(const FogNode& node, const Process& process);
    
    // Helper methods for optimization
    std::vector<int> findCandidateNodes(const std::shared_ptr<Process>& process);
    void updateNodeIndices(int nodeId);
    void buildSpatialIndices();

public: 
    static int processScoreCount;
    static int nodeScoreCount;
    static int resourceCheckCount;
    static int retryAttemptCount;
    static int partitioningAttemptCount;
    ContextAwareScheduler(const std::vector<FogNode>& nodes);
    void addProcess(std::shared_ptr<Process> process) override;
    std::shared_ptr<Process> getNextProcess() override;
    void schedule() override;
    void printSchedulingSummary() const override;
    void printSchedulingMetrics() const override;
    virtual ~ContextAwareScheduler() = default;
};

#endif // CONTEXT_AWARE_SCHEDULER_H