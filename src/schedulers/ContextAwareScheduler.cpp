#include "ContextAwareScheduler.h"
#include <iostream>
#include <cmath>
#include <limits>
#include <algorithm>
#include <queue>

// QuadTree Implementation
void QuadTree::subdivide() {
    double midX = (topLeft.x + bottomRight.x) / 2;
    double midY = (topLeft.y + bottomRight.y) / 2;
    nw = std::make_unique<QuadTree>(Point(topLeft.x, topLeft.y), Point(midX, midY));
    ne = std::make_unique<QuadTree>(Point(midX, topLeft.y), Point(bottomRight.x, midY));
    sw = std::make_unique<QuadTree>(Point(topLeft.x, midY), Point(midX, bottomRight.y));
    se = std::make_unique<QuadTree>(Point(midX, midY), Point(bottomRight.x, bottomRight.y));
}

void QuadTree::insert(double x, double y, int nodeId) {
    if (x < topLeft.x || x > bottomRight.x || y < topLeft.y || y > bottomRight.y) return;

    if (nodes.size() < CAPACITY && isLeaf()) {
        nodes.emplace_back(x, y, nodeId);
        return;
    }

    if (isLeaf()) subdivide();

    nw->insert(x, y, nodeId);
    ne->insert(x, y, nodeId);
    sw->insert(x, y, nodeId);
    se->insert(x, y, nodeId);
}

void QuadTree::queryRange(Point center, double radius, std::vector<int>& results) const {
    if (topLeft.x > center.x + radius || bottomRight.x < center.x - radius ||
        topLeft.y > center.y + radius || bottomRight.y < center.y - radius) {
        return;
    }

    for (const auto& node : nodes) {
        double dx = node.point.x - center.x;
        double dy = node.point.y - center.y;
        if (dx * dx + dy * dy <= radius * radius) {
            results.push_back(node.nodeId);
        }
    }

    if (!isLeaf()) {
        nw->queryRange(center, radius, results);
        ne->queryRange(center, radius, results);
        sw->queryRange(center, radius, results);
        se->queryRange(center, radius, results);
    }
}

// ContextAwareScheduler Implementation
ContextAwareScheduler::ContextAwareScheduler(const std::vector<FogNode>& nodes) 
    : fogNodes(nodes), sortedNodes(nodes), nodeMap(), nodeComparator(nodeMap) {
    for (auto& node : fogNodes) {
        nodeMap[node.getNodeID()] = &node;
    }
    buildSpatialIndices();
}

void ContextAwareScheduler::buildSpatialIndices() {
    nodesByCapacity.clear();
    spatialIndex = std::make_unique<QuadTree>(Point(0, 0), Point(1000, 1000)); // Assuming a 1000x1000 grid
    
    for (size_t i = 0; i < fogNodes.size(); i++) {
        const FogNode& node = fogNodes[i];
        int nodeId = node.getNodeID();
        // Assume coordinates are available; here we simulate them based on nodeId for demo
        double x = (nodeId % 100) * 10.0; // Example mapping
        double y = (nodeId / 100) * 10.0;
        spatialIndex->insert(x, y, nodeId);
        nodesByCapacity[node.getCpuCapacity()].push_back(nodeId);
    }
}

void ContextAwareScheduler::updateNodeIndices(int nodeId) {
    auto nodeIter = std::find_if(fogNodes.begin(), fogNodes.end(), 
                                 [nodeId](const FogNode& node) { return node.getNodeID() == nodeId; });
    if (nodeIter != fogNodes.end()) {
        for (auto& entry : nodesByCapacity) {
            auto& nodeList = entry.second;
            nodeList.erase(std::remove(nodeList.begin(), nodeList.end(), nodeId), nodeList.end());
        }
        double availableCapacity = nodeIter->getCpuCapacity() * (1.0 - nodeIter->getCurrentLoad());
        nodesByCapacity[availableCapacity].push_back(nodeId);
    }
}

void ContextAwareScheduler::addProcess(std::shared_ptr<Process> process) {
    BaseScheduler::addProcess(process);
    double score = calculateProcessScore(process);
    process->setProcessScore(score);
    processPriorityQueue.push(process);
    std::cout << "Process " << process->getProcessID() << " added to queue with score " << score << ".\n";
}

double ContextAwareScheduler::calculateLocationScore(const std::shared_ptr<Process>& process, const FogNode& node) {
    // Simulate coordinates for process and node (in practice, these would be class members)
    double px = (process->getProcessID() % 100) * 10.0; // Example mapping
    double py = (process->getProcessID() / 100) * 10.0;
    double nx = (node.getNodeID() % 100) * 10.0;
    double ny = (node.getNodeID() / 100) * 10.0;
    double distance = std::sqrt((px - nx) * (px - nx) + (py - ny) * (py - ny));
    return 1.0 / (1.0 + distance); // Higher score for closer nodes
}

double ContextAwareScheduler::calculateLoadBalanceScore(const FogNode& node) {
    return 1.0 / (1.0 + std::exp(std::min(node.getCurrentLoad() * 5, 10.0)));
}

double ContextAwareScheduler::calculateProcessScore(const std::shared_ptr<Process>& process) {
    return (1.0 / (1.0 + process->getPriority())) + 
           (0.5 * (1.0 - process->getMobility())) + 
           (0.2 * process->getNps()) + 
           (0.3 * (1.0 - process->getRelinquishProbability())) + 
           (0.4 * process->getLatencySensitivity());
}

double ContextAwareScheduler::calculateNodeScore(const FogNode& node, const std::shared_ptr<Process>& process) {
    double locationScore = calculateLocationScore(process, node);
    double loadBalanceScore = calculateLoadBalanceScore(node);
    return (1.0 / (1.0 + node.getDelay())) + 
           (node.getBandwidth() / process->getRequiredBandwidth()) + 
           (1.0 - (node.getPacketLoss() / process->getMaxPacketLoss())) + 
           locationScore + 
           loadBalanceScore;
}

double ContextAwareScheduler::calculateNewLoad(const FogNode& node, const Resource& resources) {
    return node.getCurrentLoad() + (resources.cpu / node.getCpuCapacity());
}

bool ContextAwareScheduler::canNodeHandleProcess(const FogNode& node, const Process& process) {
    return node.getDelay() <= process.getMaxDelay() && node.getPacketLoss() <= process.getMaxPacketLoss();
}

bool ContextAwareScheduler::canResourcesFit(const FogNode& node, const Resource& resources, const Process& process) {
    return (node.getCpuCapacity() >= resources.cpu * 0.5) && 
           node.getMemory() >= resources.memory && 
           node.getBandwidth() >= process.getRequiredBandwidth();
}

std::vector<int> ContextAwareScheduler::findCandidateNodes(const std::shared_ptr<Process>& process) {
    std::vector<int> candidates;
    // Simulate process coordinates
    double px = (process->getProcessID() % 100) * 10.0;
    double py = (process->getProcessID() / 100) * 10.0;
    spatialIndex->queryRange(Point(px, py), 100.0, candidates); // 100 units radius
    return candidates;
}

std::shared_ptr<Process> ContextAwareScheduler::getNextProcess() {
    if (processPriorityQueue.empty()) return nullptr;
    auto process = processPriorityQueue.top();
    processPriorityQueue.pop();
    return process;
}

int ContextAwareScheduler::assignToFogNode(std::shared_ptr<Process> process) {
    int bestNode = -1;
    double bestScore = std::numeric_limits<double>::lowest();
    std::vector<int> candidateNodeIds = findCandidateNodes(process);
    
    for (int nodeId : candidateNodeIds) {
        FogNode& node = *nodeMap[nodeId];
        if (!node.getIsActive()) continue;
        auto resources = process->getRequiredResources();
        if (canResourcesFit(node, resources, *process) && canNodeHandleProcess(node, *process)) {
            double newLoad = calculateNewLoad(node, resources);
            if (newLoad <= 1.0) {
                double score = calculateNodeScore(node, process);
                std::cout << "Fog Node " << nodeId << " score: " << score << " (newLoad: " << newLoad << ")\n";
                if (score > bestScore) {
                    bestScore = score;
                    bestNode = nodeId;
                }
            }
        }
    }
    return bestNode;
}

bool ContextAwareScheduler::partitionProcess(std::shared_ptr<Process> process) {
    auto resources = process->getRequiredResources();
    double remainingCpu = resources.cpu;
    double remainingMem = resources.memory;
    double remainingBw = process->getRequiredBandwidth();
    std::vector<int> assignedNodes;

    std::set<int, NodeLoadComparator> availableNodes(nodeComparator);
    for (const auto& node : fogNodes) {
        if (node.getIsActive() && node.getCurrentLoad() < 1.0) {
            availableNodes.insert(node.getNodeID());
        }
    }

    while (remainingCpu > 0 || remainingMem > 0 || remainingBw > 0) {
        if (availableNodes.empty()) break;
        int nodeId = *availableNodes.begin();
        availableNodes.erase(availableNodes.begin());
        FogNode& node = *nodeMap[nodeId];

        double cpuAvailable = node.getAvailableCpu();
        double memAvailable = node.getAvailableMemory();
        double bwAvailable = node.getBandwidth();

        if (canNodeHandleProcess(node, *process)) {
            double cpuToAssign = std::min(remainingCpu, cpuAvailable);
            double memToAssign = std::min(remainingMem, memAvailable);
            double bwToAssign = std::min(remainingBw, bwAvailable);

            if (cpuToAssign > 0 && memToAssign > 0 && bwToAssign > 0) {
                auto partitionedProcess = std::make_shared<Process>(*process);
                partitionedProcess->setRequiredResources(cpuToAssign, memToAssign);
                if (node.assignProcess(*partitionedProcess)) {
                    processToNodeMap[process->getProcessID()].push_back(nodeId);
                    assignedNodes.push_back(nodeId);
                    remainingCpu -= cpuToAssign;
                    remainingMem -= memToAssign;
                    remainingBw -= bwToAssign;
                    std::cout << "Partitioned Process " << process->getProcessID() 
                              << " to Node " << nodeId << " (CPU: " << cpuToAssign << ")\n";
                    updateNodeIndices(nodeId);
                    if (node.getCurrentLoad() < 1.0) availableNodes.insert(nodeId);
                }
            }
        }
    }

    if (remainingCpu <= 0 && remainingMem <= 0 && remainingBw <= 0) {
        std::cout << "Process " << process->getProcessID() << " fully partitioned: ";
        for (int id : assignedNodes) std::cout << id << " ";
        std::cout << "\n";
        return true;
    }
    std::cout << "Partitioning failed for Process " << process->getProcessID() 
              << ". Remaining: CPU=" << remainingCpu << "\n";
    return false;
}

void ContextAwareScheduler::processRetryQueue() {
    while (!retryQueue.empty()) {
        auto process = retryQueue.top();
        retryQueue.pop();
        if (!partitionProcess(process)) {
            std::cout << "Retry failed for Process " << process->getProcessID() << ".\n";
        } else {
            std::cout << "Retry succeeded for Process " << process->getProcessID() << ".\n";
        }
    }
}

void ContextAwareScheduler::schedule() {
    std::cout << "Scheduling started. Queue size: " << processPriorityQueue.size() << "\n";
    if (processPriorityQueue.empty()) return;

    sortedNodes = fogNodes;
    std::sort(sortedNodes.begin(), sortedNodes.end(), 
              [](const FogNode& a, const FogNode& b) { return a.getCurrentLoad() < b.getCurrentLoad(); });

    while (!processPriorityQueue.empty()) {
        auto process = getNextProcess();
        int assignedNode = assignToFogNode(process);
        if (assignedNode != -1) {
            FogNode& node = *nodeMap[assignedNode];
            if (node.assignProcess(*process)) {
                std::cout << "Process " << process->getProcessID() << " assigned to Node " << assignedNode << ".\n";
                processToNodeMap[process->getProcessID()] = {assignedNode};
                updateNodeIndices(assignedNode);
            } else if (!partitionProcess(process)) {
                std::cout << "Assignment failed. Adding to retry queue.\n";
                retryQueue.push(process);
            }
        } else if (!partitionProcess(process)) {
            std::cout << "No node found. Adding to retry queue.\n";
            retryQueue.push(process);
        }
    }
    processRetryQueue();
}

void ContextAwareScheduler::printSchedulingState() const {
    std::cout << "Current Scheduling State:\n";
    for (const auto& entry : processToNodeMap) {
        std::cout << "Process " << entry.first << " -> Node(s): ";
        for (int nodeId : entry.second) std::cout << nodeId << " ";
        std::cout << "\n";
    }
}