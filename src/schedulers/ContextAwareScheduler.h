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
#include <queue>
#include <set>

// Custom comparator for priority queue
struct ProcessScoreComparator {
    bool operator()(const std::shared_ptr<Process>& a, const std::shared_ptr<Process>& b) const {
        return a->getProcessScore() < b->getProcessScore(); // Higher score has higher priority (max-heap)
    }
};

// Simple 2D Point structure
struct Point {
    double x, y;
    Point(double x_ = 0, double y_ = 0) : x(x_), y(y_) {}
};

// QuadTree Node structure
struct QuadTreeNode {
    Point point;              // Node's coordinates
    int nodeId;               // FogNode ID
    QuadTreeNode(double x, double y, int id) : point(x, y), nodeId(id) {}
};

// QuadTree class for spatial indexing
class QuadTree {
private:
    static constexpr int CAPACITY = 4; // Max nodes per QuadTree node before splitting
    Point topLeft, bottomRight;        // Boundary of this QuadTree node
    std::vector<QuadTreeNode> nodes;   // Nodes stored in this QuadTree node
    std::unique_ptr<QuadTree> nw, ne, sw, se; // Child quadrants

    bool isLeaf() const { return !nw; }
    void subdivide();

public:
    QuadTree(Point tl, Point br) : topLeft(tl), bottomRight(br) {}
    void insert(double x, double y, int nodeId);
    void queryRange(Point center, double radius, std::vector<int>& results) const;
};

class ContextAwareScheduler : public BaseScheduler {
private:
    std::vector<FogNode> fogNodes;
    std::vector<FogNode> sortedNodes;  // Pre-sorted nodes for performance
    
    // Priority queue for processes
    std::priority_queue<std::shared_ptr<Process>, std::vector<std::shared_ptr<Process>>, ProcessScoreComparator> processPriorityQueue;
    
    // Retry queue for failed assignments
    std::priority_queue<std::shared_ptr<Process>, std::vector<std::shared_ptr<Process>>, ProcessScoreComparator> retryQueue;
    
    // Maps process ID to list of fog node IDs
    std::map<int, std::vector<int>> processToNodeMap;
    
    // Spatial index using QuadTree (replaces nodesByLocation)
    std::unique_ptr<QuadTree> spatialIndex;
    std::map<double, std::vector<int>> nodesByCapacity; // Maps capacity to node IDs
    std::map<int, FogNode*> nodeMap;                    // Maps node ID to FogNode pointer
    
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

    // Override the inherited getNextProcess from BaseScheduler
    std::shared_ptr<Process> getNextProcess() override;
    
    // Node assignment and partitioning methods
    int assignToFogNode(std::shared_ptr<Process> process);
    bool partitionProcess(std::shared_ptr<Process> process);
    
    // Process retry queue
    void processRetryQueue();

    // Helper methods for scoring and evaluation
    double calculateLocationScore(const std::shared_ptr<Process>& process, const FogNode& node);
    double calculateLoadBalanceScore(const FogNode& node);
    double calculateProcessScore(const std::shared_ptr<Process>& process);
    double calculateNodeScore(const FogNode& node, const std::shared_ptr<Process>& process);
    double calculateNewLoad(const FogNode& node, const Resource& resources);
    bool canResourcesFit(const FogNode& node, const Resource& resources, const Process& process);
    bool canNodeHandleProcess(const FogNode& node, const Process& process);
    
    // New helper methods for optimization
    std::vector<int> findCandidateNodes(const std::shared_ptr<Process>& process);
    void updateNodeIndices(int nodeId);
    void buildSpatialIndices();

public:
    ContextAwareScheduler(const std::vector<FogNode>& nodes);
    void addProcess(std::shared_ptr<Process> process) override;
    void schedule() override;
    void printSchedulingState() const;
    virtual ~ContextAwareScheduler() = default;
};

#endif // CONTEXT_AWARE_SCHEDULER_H