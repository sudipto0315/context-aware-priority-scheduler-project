#include "ContextAwareScheduler.h"
#include <iostream>
#include <cmath>
#include <limits>
#include <queue>
#include <unordered_map>
#include <set>

/* Quadtree methods section */

// Subdivide quadtree node into four quadrants
void QuadTree::subdivide() {
    double midX = (topLeft.x + bottomRight.x) / 2;
    double midY = (topLeft.y + bottomRight.y) / 2;
    nw = std::make_unique<QuadTree>(Point(topLeft.x, topLeft.y), Point(midX, midY));
    ne = std::make_unique<QuadTree>(Point(midX, topLeft.y), Point(bottomRight.x, midY));
    sw = std::make_unique<QuadTree>(Point(topLeft.x, midY), Point(midX, bottomRight.y));
    se = std::make_unique<QuadTree>(Point(midX, midY), Point(bottomRight.x, bottomRight.y));
}

// Insert node into quadtree
void QuadTree::insert(double x, double y, int nodeId) {
    if (x < topLeft.x || x > bottomRight.x || y < topLeft.y || y > bottomRight.y) return;

    if (nodes.size() < CAPACITY && isLeaf()) {
        nodes.emplace_back(x, y, nodeId);
        return;
    }

    if (isLeaf()) subdivide();

    double midX = (topLeft.x + bottomRight.x) / 2;
    double midY = (topLeft.y + bottomRight.y) / 2;
    
    if (x < midX) {
        if (y < midY) nw->insert(x, y, nodeId);
        else sw->insert(x, y, nodeId);
    } else {
        if (y < midY) ne->insert(x, y, nodeId);
        else se->insert(x, y, nodeId);
    }
}

// Query nodes within a given radius
void QuadTree::queryRange(Point center, double radius, std::vector<int>& results, int maxResults) const {
    if (maxResults > 0 && static_cast<int>(results.size()) >= maxResults) return;

    if (topLeft.x > center.x + radius || bottomRight.x < center.x - radius ||
        topLeft.y > center.y + radius || bottomRight.y < center.y - radius) {
        return;
    }

    for (const auto& node : nodes) {
        double dx = node.point.x - center.x;
        double dy = node.point.y - center.y;
        if (dx * dx + dy * dy <= radius * radius) {
            results.push_back(node.nodeId);
            if (maxResults > 0 && static_cast<int>(results.size()) >= maxResults) return;
        }
    }

    if (!isLeaf()) {
        double midX = (topLeft.x + bottomRight.x) / 2;
        double midY = (topLeft.y + bottomRight.y) / 2;
        bool isWest = center.x < midX;
        bool isNorth = center.y < midY;
        
        if (isWest && isNorth) {
            nw->queryRange(center, radius, results, maxResults);
            if (maxResults > 0 && static_cast<int>(results.size()) >= maxResults) return;
            ne->queryRange(center, radius, results, maxResults);
            if (maxResults > 0 && static_cast<int>(results.size()) >= maxResults) return;
            sw->queryRange(center, radius, results, maxResults);
            if (maxResults > 0 && static_cast<int>(results.size()) >= maxResults) return;
            se->queryRange(center, radius, results, maxResults);
        } else if (!isWest && isNorth) {
            ne->queryRange(center, radius, results, maxResults);
            if (maxResults > 0 && static_cast<int>(results.size()) >= maxResults) return;
            nw->queryRange(center, radius, results, maxResults);
            if (maxResults > 0 && static_cast<int>(results.size()) >= maxResults) return;
            se->queryRange(center, radius, results, maxResults);
            if (maxResults > 0 && static_cast<int>(results.size()) >= maxResults) return;
            sw->queryRange(center, radius, results, maxResults);
        } else if (isWest && !isNorth) {
            sw->queryRange(center, radius, results, maxResults);
            if (maxResults > 0 && static_cast<int>(results.size()) >= maxResults) return;
            nw->queryRange(center, radius, results, maxResults);
            if (maxResults > 0 && static_cast<int>(results.size()) >= maxResults) return;
            se->queryRange(center, radius, results, maxResults);
            if (maxResults > 0 && static_cast<int>(results.size()) >= maxResults) return;
            ne->queryRange(center, radius, results, maxResults);
        } else {
            se->queryRange(center, radius, results, maxResults);
            if (maxResults > 0 && static_cast<int>(results.size()) >= maxResults) return;
            ne->queryRange(center, radius, results, maxResults);
            if (maxResults > 0 && static_cast<int>(results.size()) >= maxResults) return;
            sw->queryRange(center, radius, results, maxResults);
            if (maxResults > 0 && static_cast<int>(results.size()) >= maxResults) return;
            nw->queryRange(center, radius, results, maxResults);
        }
    }
}

/* Constructor and initialization section */

// Constructor
ContextAwareScheduler::ContextAwareScheduler(const std::vector<FogNode>& nodes) 
    : fogNodes(nodes), sortedNodes(nodes), nodeMap(), nodeComparator(nodeMap), needsResorting(false) {
    for (size_t i = 0; i < fogNodes.size(); i++) {
        int nodeId = fogNodes[i].getNodeID();
        nodeMap[nodeId] = &fogNodes[i];
        nodeIndexMap[nodeId] = i;
    }
    
    for (const auto& node : fogNodes) {
        int nodeId = node.getNodeID();
        double x = (nodeId % 100) * 10.0;
        double y = (nodeId / 100) * 10.0;
        nodeCoordinates[nodeId] = {x, y};
    }
    
    buildSpatialIndices();
    std::cout << "ContextAwareScheduler initialized with " << fogNodes.size() << " nodes.\n";
}

// Build spatial indices for fog nodes
void ContextAwareScheduler::buildSpatialIndices() {
    double minX = std::numeric_limits<double>::max(), minY = minX;
    double maxX = std::numeric_limits<double>::lowest(), maxY = maxX;
    
    for (const auto& node : fogNodes) {
        int nodeId = node.getNodeID();
        double x = nodeCoordinates[nodeId].first;
        double y = nodeCoordinates[nodeId].second;
        minX = std::min(minX, x);
        minY = std::min(minY, y);
        maxX = std::max(maxX, x);
        maxY = std::max(maxY, y);
        
        nodesByCapacity.emplace(node.getCpuCapacity(), nodeId);
        nodesByMemory.emplace(node.getMemory(), nodeId);
        nodesByBandwidth.emplace(node.getBandwidth(), nodeId);
    }
    
    minX -= 10.0; minY -= 10.0;
    maxX += 10.0; maxY += 10.0;
    
    spatialIndex = std::make_unique<QuadTree>(Point(minX, minY), Point(maxX, maxY));
    for (const auto& node : fogNodes) {
        int nodeId = node.getNodeID();
        spatialIndex->insert(nodeCoordinates[nodeId].first, nodeCoordinates[nodeId].second, nodeId);
    }
    std::cout << "Spatial indices built with boundaries: (" << minX << ", " << minY << ") to (" << maxX << ", " << maxY << ").\n"; 
    std::cout << "Spatial dimensions: (" << maxX - minX << ", " << maxY - minY << ").\n"; 
    std::cout << "Spatial boundary (A,B,C,D): (" << minX << ", " << minY << "), (" << maxX << ", " << minY << "), (" << maxX << ", " << maxY << "), (" << minX << ", " << maxY << ")\n";
}

/* Process management section */

// Add process to the queue
void ContextAwareScheduler::addProcess(std::shared_ptr<Process> process) {
    BaseScheduler::addProcess(process);
    double score = calculateProcessScore(process);
    process->setProcessScore(score);
    
    int priority = process->getPriority();
    double latencySensitivity = process->getLatencySensitivity();
    double resourceNeeds = process->getRequiredResources().cpu + process->getRequiredResources().memory;
    int groupHash = static_cast<int>((priority * 100) + (latencySensitivity * 10) + (resourceNeeds / 10)); // used for grouping processes why Hash? reason: to group processes that have the same priority, latency sensitivity, and resource needs
    processesByGroup[groupHash].push_back(process);
    
    processPriorityQueue.push(process);
    std::cout << "Process " << process->getProcessID() << " added to queue with score " << score 
              << " (groupHash: " << groupHash << ").\n";
}

/* Calculations and scoring methods section */

// Update node indices for resource tracking
void ContextAwareScheduler::updateNodeIndices(int nodeId) {
    if (nodeIndexMap.find(nodeId) == nodeIndexMap.end()) return;
    
    const FogNode& node = *nodeMap[nodeId];
    double oldCapacity = node.getCpuCapacity();  // Assume we need to track the old value
    double availableCapacity = node.getCpuCapacity() * (1.0 - node.getCurrentLoad());
    
    // Erase the specific node entry
    nodesByCapacity.erase({oldCapacity, nodeId});
    nodesByMemory.erase({node.getMemory(), nodeId});
    nodesByBandwidth.erase({node.getBandwidth(), nodeId});
    
    nodesByCapacity.emplace(availableCapacity, nodeId);
    nodesByMemory.emplace(node.getAvailableMemory(), nodeId);
    nodesByBandwidth.emplace(node.getBandwidth(), nodeId);
    std::cout << "Node " << nodeId << " indices updated. New capacity: " << availableCapacity << "\n";
}

// Calculate location-based scoring using quadtree
double ContextAwareScheduler::calculateLocationScore(const std::shared_ptr<Process>& process, const FogNode& node) {
    int processId = process->getProcessID();
    int nodeId = node.getNodeID();
    
    auto cacheKey = std::make_pair(processId, nodeId);
    auto cacheIter = locationScoreCache.find(cacheKey);
    if (cacheIter != locationScoreCache.end()) {
        return cacheIter->second;
    }
    
    double px = (processId % 100) * 10.0;
    double py = (processId / 100) * 10.0;
    const auto& nodeCoord = nodeCoordinates[nodeId];
    double nx = nodeCoord.first;
    double ny = nodeCoord.second;
    
    double distance = std::sqrt((px - nx) * (px - nx) + (py - ny) * (py - ny));
    double score = 1.0 / (1.0 + distance);
    
    locationScoreCache[cacheKey] = score;
    std::cout << "Location score for Process " << processId << " and Node " << nodeId << ": " << score << "\n";
    return score;
}

// Calculate load balance scoring
double ContextAwareScheduler::calculateLoadBalanceScore(const FogNode& node) {
    int nodeId = node.getNodeID();
    
    auto cacheIter = loadBalanceCache.find(nodeId);
    if (cacheIter != loadBalanceCache.end() && 
        std::abs(cacheIter->second.first - node.getCurrentLoad()) < 0.01) {
        return cacheIter->second.second;
    }
    
    double score = 1.0 / (1.0 + std::exp(std::min(node.getCurrentLoad() * 5, 10.0)));
    
    loadBalanceCache[nodeId] = {node.getCurrentLoad(), score};
    std::cout << "Load balance score for Node " << nodeId << ": " << score << "\n";
    return score;
}

// Calculate comprehensive process score
double ContextAwareScheduler::calculateProcessScore(const std::shared_ptr<Process>& process) {
    int processId = process->getProcessID();
    
    auto cacheIter = processScoreCache.find(processId);
    if (cacheIter != processScoreCache.end()) {
        return cacheIter->second;
    }
    
    double score = (1.0 / (1.0 + process->getPriority())) + // Priority consideration
                   (0.5 * (1.0 - process->getMobility())) + // Mobility factor
                   (0.2 * process->getNps()) + // Network Performance Score
                   (0.3 * (1.0 - process->getRelinquishProbability())) + // Stability factor
                   (0.4 * process->getLatencySensitivity()); // Latency sensitivity
    
    processScoreCache[processId] = score;
    std::cout << "Process score for Process " << processId << ": " << score << "\n";
    return score;
}

// Calculate comprehensive node selection score
double ContextAwareScheduler::calculateNodeScore(const FogNode& node, const std::shared_ptr<Process>& process) {
    int nodeId = node.getNodeID();
    int processId = process->getProcessID();
    
    auto cacheKey = std::make_pair(nodeId, processId);
    auto cacheIter = nodeScoreCache.find(cacheKey);
    if (cacheIter != nodeScoreCache.end()) {
        return cacheIter->second;
    }
    
    double locationScore = calculateLocationScore(process, node);
    double loadBalanceScore = calculateLoadBalanceScore(node);
    
    double score = (1.0 / (1.0 + node.getDelay())) + // Delay preference
                   (node.getBandwidth() / process->getRequiredBandwidth()) + // Bandwidth utilization
                   (1.0 - (node.getPacketLoss() / process->getMaxPacketLoss())) + // Packet loss
                   locationScore + // Location matching
                   loadBalanceScore; // Load distribution
    
    nodeScoreCache[cacheKey] = score;
    std::cout << "Node score for Node " << nodeId << " and Process " << processId << ": " << score << "\n";
    return score;
}

// Calculate new load after assigning process to node
double ContextAwareScheduler::calculateNewLoad(const FogNode& node, const Resource& resources) {
    double newLoad = node.getCurrentLoad() + (resources.cpu / node.getCpuCapacity());
    std::cout << "New load calculated for Node " << node.getNodeID() << ": " << newLoad << "\n";
    return newLoad;
}

// Check if a node can handle a process
bool ContextAwareScheduler::canNodeHandleProcess(const FogNode& node, const Process& process) {
    bool canHandle = node.getDelay() <= process.getMaxDelay() && 
                     node.getPacketLoss() <= process.getMaxPacketLoss();
    std::cout << "Node " << node.getNodeID() << " can handle Process " << process.getProcessID() << ": " << (canHandle ? "Yes" : "No") << "\n";
    return canHandle;
}

// Check if resources can fit a process
bool ContextAwareScheduler::canResourcesFit(const FogNode& node, const Resource& resources, const Process& process) {
    bool fits = (node.getCpuCapacity() >= resources.cpu * 0.5) && // Partial assignment threshold
                node.getMemory() >= resources.memory && 
                node.getBandwidth() >= process.getRequiredBandwidth();
    std::cout << "Resources fit check for Node " << node.getNodeID() << " and Process " << process.getProcessID() << ": " << (fits ? "Yes" : "No") << "\n";
    return fits;
}

// Find candidate nodes using quadtree and resource constraints
std::vector<int> ContextAwareScheduler::findCandidateNodes(const std::shared_ptr<Process>& process) {
    std::vector<int> candidates;
    const auto& resources = process->getRequiredResources();
    
    // Use lower_bound to find nodes with sufficient resources
    auto cpuIt = nodesByCapacity.lower_bound({resources.cpu * 0.5, 0});
    auto memIt = nodesByMemory.lower_bound({resources.memory, 0});
    auto bwIt = nodesByBandwidth.lower_bound({process->getRequiredBandwidth(), 0});
    
    double px = (process->getProcessID() % 100) * 10.0;
    double py = (process->getProcessID() / 100) * 10.0;
    double radius = 100.0 + (10.0 * process->getLatencySensitivity());
    spatialIndex->queryRange(Point(px, py), radius, candidates, 20);
    
    std::vector<int> finalCandidates;
    for (int nodeId : candidates) {
        const FogNode& node = *nodeMap[nodeId];
        // Directly check resource availability instead of relying on iterator position
        if (node.getCpuCapacity() >= resources.cpu * 0.5 &&
            node.getMemory() >= resources.memory &&
            node.getBandwidth() >= process->getRequiredBandwidth()) {
            finalCandidates.push_back(nodeId);
        }
    }
    std::cout << "Found " << finalCandidates.size() << " candidate nodes for Process " << process->getProcessID() << ".\n";
    return finalCandidates;
}

/* Algorithm methods section */

// Select next process based on priority queue
std::shared_ptr<Process> ContextAwareScheduler::getNextProcess() {
    if (processPriorityQueue.empty()) {
        std::cout << "Process queue is empty.\n";
        return nullptr;
    }
    auto process = processPriorityQueue.top();
    processPriorityQueue.pop();
    std::cout << "Retrieved Process " << process->getProcessID() << " from queue.\n";
    return process;
}

// Assign process to the most suitable fog node
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
                if (score > bestScore) {
                    bestScore = score;
                    bestNode = nodeId;
                    if (score > 4.0) break; // Early exit for high scores
                }
            }
        }
    }
    if (bestNode != -1) {
        std::cout << "Best node for Process " << process->getProcessID() << " is Node " << bestNode << " with score " << bestScore << ".\n";
    } else {
        std::cout << "No suitable node found for Process " << process->getProcessID() << ".\n";
    }
    return bestNode;
}

// Partition process across multiple nodes
bool ContextAwareScheduler::partitionProcess(std::shared_ptr<Process> process) {
    auto resources = process->getRequiredResources();
    double remainingCpu = resources.cpu;
    double remainingMem = resources.memory;
    double remainingBw = process->getRequiredBandwidth();
    std::vector<int> assignedNodes;
    
    std::priority_queue<std::pair<double, int>> nodeQueue;
    for (const auto& node : fogNodes) {
        if (node.getIsActive() && node.getCurrentLoad() < 1.0 && canNodeHandleProcess(node, *process)) {
            double score = (0.6 * node.getAvailableCpu() / remainingCpu) + // CPU contribution
                          (0.3 * node.getAvailableMemory() / remainingMem) + // Memory contribution
                          (0.1 * node.getBandwidth() / remainingBw); // Bandwidth contribution
            nodeQueue.emplace(score, node.getNodeID());
        }
    }
    
    const double MIN_CPU_ALLOCATION = 0.1;
    while (!nodeQueue.empty() && remainingCpu > 0.001) {
        int nodeId = nodeQueue.top().second;
        nodeQueue.pop();
        FogNode& node = *nodeMap[nodeId];
        
        double cpuToAssign = std::min(node.getAvailableCpu(), remainingCpu);
        double memToAssign = std::min(node.getAvailableMemory(), remainingMem);
        double bwToAssign = std::min(node.getBandwidth(), remainingBw);
        
        if (cpuToAssign < MIN_CPU_ALLOCATION) continue;
        
        auto partitionedProcess = std::make_shared<Process>(*process);
        partitionedProcess->setRequiredResources(cpuToAssign, memToAssign);
        
        if (node.assignProcess(*partitionedProcess)) {
            processToNodeMap[process->getProcessID()].push_back(nodeId);
            assignedNodes.push_back(nodeId);
            remainingCpu -= cpuToAssign;
            remainingMem -= memToAssign;
            remainingBw -= bwToAssign;
            updateNodeIndices(nodeId);
            std::cout << "Assigned " << cpuToAssign << " CPU to Node " << nodeId << " for Process " << process->getProcessID() << ".\n";
        }
    }
    
    if (remainingCpu <= 0.001 && remainingMem <= 0.001 && remainingBw <= 0.001) {
        std::cout << "Process " << process->getProcessID() << " fully partitioned across " 
                  << assignedNodes.size() << " nodes\n";
        return true;
    }
    std::cout << "Partitioning failed for Process " << process->getProcessID() 
              << ". Remaining: CPU=" << remainingCpu << "\n";
    return false;
}

// Process retry queue for failed assignments
void ContextAwareScheduler::processRetryQueue() {
    const int MAX_RETRY_ATTEMPTS = 3;
    std::unordered_map<int, int> retryAttempts;
    
    while (!retryQueue.empty()) {
        auto process = retryQueue.top();
        retryQueue.pop();
        
        int processId = process->getProcessID();
        retryAttempts[processId]++;
        
        if (retryAttempts[processId] > MAX_RETRY_ATTEMPTS) {
            std::cout << "Max retry attempts reached for Process " << processId << ".\n";
            allProcesses.push_back({process, {}, false}); // Add as failed
            continue;
        }
        if (partitionProcess(process)) {
            scheduledProcesses.push_back({process, processToNodeMap[processId]});
            allProcesses.push_back({process, processToNodeMap[processId], true});
            std::cout << "Retry succeeded for Process " << processId << ".\n";
        } else {
            std::cout << "Retry failed for Process " << processId 
                      << " (attempt " << retryAttempts[processId] << ").\n";
            allProcesses.push_back({process, {}, false}); // Add as failed
        }
    }
    std::cout << "Retry queue processing completed.\n";
}

// Main scheduling method
void ContextAwareScheduler::schedule() {
    std::cout << "Scheduling started. Queue size: " << processPriorityQueue.size() << "\n";
    if (processPriorityQueue.empty()) return;
    
    std::vector<std::shared_ptr<Process>> processes;
    while (!processPriorityQueue.empty()) {
        processes.push_back(getNextProcess());
    }
    
    for (size_t i = 0; i < processes.size(); i++) {
        auto process = processes[i];
        
        int assignedNode = assignToFogNode(process);
        
        if (assignedNode != -1) {
            FogNode& node = *nodeMap[assignedNode];
            if (node.assignProcess(*process)) {
                std::cout << "Process " << process->getProcessID() << " assigned to Node " << assignedNode << ".\n";
                processToNodeMap[process->getProcessID()] = {assignedNode};
                updateNodeIndices(assignedNode);
                scheduledProcesses.push_back({process, {assignedNode}}); 
                allProcesses.push_back({process, {assignedNode}, true});
            } else if (partitionProcess(process)) {
                scheduledProcesses.push_back({process, processToNodeMap[process->getProcessID()]});
                allProcesses.push_back({process, processToNodeMap[process->getProcessID()], true});
            } else {
                std::cout << "Assignment failed. Adding to retry queue.\n";
                retryQueue.push(process);
                allProcesses.push_back({process, {}, false}); // Add as failed
            }
        } else if (partitionProcess(process)) {
            scheduledProcesses.push_back({process, processToNodeMap[process->getProcessID()]});
            allProcesses.push_back({process, processToNodeMap[process->getProcessID()], true});
        } else {
            std::cout << "No node found. Adding to retry queue.\n";
            retryQueue.push(process);
            allProcesses.push_back({process, {}, false}); // Add as failed
        }
        
        if (i % 100 == 0) {
            if (nodeScoreCache.size() > 1000) {
                nodeScoreCache.clear();
                locationScoreCache.clear();
                std::cout << "Caches cleared at iteration " << i << ".\n";
            }
        }
    }
    
    processRetryQueue();
    std::cout << "Scheduling completed.\n";
}

/* Utility methods section */

// Print summary of scheduled processes
void ContextAwareScheduler::printSchedulingSummary() const {
    std::cout << "\n=== Scheduled Processes Summary ===\n";
    for (size_t i = 0; i < allProcesses.size(); ++i) {
        const auto& [process, nodeIds, isScheduled] = allProcesses[i];
        std::cout << "Process " << process->getProcessID() 
                  << " (Process score: " << process->getProcessScore() << ") ";
        if (i == 0) {
            std::cout << "is scheduled first and ";
        }
        if (!isScheduled) {
            std::cout << "not scheduled.\n";
        } else if (nodeIds.size() == 1) {
            std::cout << "assigned to Node " << nodeIds[0] << ".\n";
        } else {
            std::cout << "partitioned across nodes: ";
            for (size_t j = 0; j < nodeIds.size(); ++j) {
                std::cout << nodeIds[j];
                if (j < nodeIds.size() - 1) std::cout << ", ";
            }
            std::cout << ".\n";
        }
    }
    std::cout << "==================================\n";
}



// Print the current scheduling state
void ContextAwareScheduler::printSchedulingState() const {
    std::cout << "Current Scheduling State:\n";
    for (const auto& entry : processToNodeMap) {
        std::cout << "Process " << entry.first << " -> Node(s): ";
        for (int nodeId : entry.second) std::cout << nodeId << " ";
        std::cout << "\n";
    }
}