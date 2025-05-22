#include "ContextAwareScheduler.h"
#include <iostream>
#include <cmath>
#include <limits>
#include <queue>
#include <unordered_map>
#include <set>
#include <random>
#include <stdexcept>

// Add calculation counters as member variables (declared in .h)
int ContextAwareScheduler::processScoreCount = 0;
int ContextAwareScheduler::nodeScoreCount = 0;
int ContextAwareScheduler::resourceCheckCount = 0;
int ContextAwareScheduler::retryAttemptCount = 0;
int ContextAwareScheduler::partitioningAttemptCount = 0;

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
    if (x < topLeft.x || x > bottomRight.x || y < topLeft.y || y > bottomRight.y) {
        std::cerr << "Point (" << x << ", " << y << ") out of bounds for nodeId " << nodeId << std::endl;
        return;
    }

    if (nodes.size() < CAPACITY && isLeaf()) {
        nodes.emplace_back(x, y, nodeId);
        return;
    }

    if (isLeaf()) subdivide();

    double midX = (topLeft.x + bottomRight.x) / 2;
    double midY = (topLeft.y + bottomRight.y) / 2;
    
    if (x <= midX) {
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
        
        // Process quadrants in order of proximity
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
    // Initialize node mappings
    for (size_t i = 0; i < fogNodes.size(); i++) {
        int nodeId = fogNodes[i].getNodeID();
        nodeMap[nodeId] = &fogNodes[i];
        nodeIndexMap[nodeId] = i;
        nodeUsedBandwidth[nodeId] = 0.0; // Initialize used bandwidth for each node
    }
    
    // Define location-to-coordinate mapping
    std::unordered_map<std::string, Point> locationToCoords = {
        {"Zone_A", Point(10.0, 10.0)},
        {"Zone_B", Point(20.0, 10.0)},
        {"Zone_C", Point(10.0, 20.0)},
        {"Zone_D", Point(20.0, 20.0)}
    };

    // Random offset generator for nodes in the same zone
    std::random_device rd;
    std::default_random_engine generator(rd());
    std::uniform_real_distribution<double> distribution(-5.0, 5.0);

    // Assign coordinates based on node location
    for (const auto& node : fogNodes) {
        int nodeId = node.getNodeID();
        std::string location = node.getLocation();
        if (locationToCoords.find(location) == locationToCoords.end()) {
            throw std::runtime_error("Unknown location: " + location + " for Node " + std::to_string(nodeId));
        }
        Point baseCoords = locationToCoords[location];
        double offsetX = distribution(generator);
        double offsetY = distribution(generator);
        nodeCoordinates[nodeId] = {baseCoords.x + offsetX, baseCoords.y + offsetY};
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
        
        nodesByCapacity.emplace(node.getAvailableCpu(), nodeId);
        nodesByMemory.emplace(node.getAvailableMemory(), nodeId);
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
    auto hash = std::hash<std::string>{}(std::to_string(priority) + "_" +
                                         std::to_string(latencySensitivity) + "_" +
                                         std::to_string(resourceNeeds));
    int groupHash = static_cast<int>(hash % std::numeric_limits<int>::max());
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

    // Define location-to-coordinate mapping for processes
    std::unordered_map<std::string, Point> locationToCoords = {
        {"Zone_A", Point(10.0, 10.0)},
        {"Zone_B", Point(20.0, 10.0)},
        {"Zone_C", Point(10.0, 20.0)},
        {"Zone_D", Point(20.0, 20.0)}
    };

    std::string processLocation = process->getRequestLocation();
    if (locationToCoords.find(processLocation) == locationToCoords.end()) {
        throw std::runtime_error("Unknown process location: " + processLocation + " for Process " + std::to_string(processId));
    }
    Point pCoords = locationToCoords[processLocation];
    double px = pCoords.x;
    double py = pCoords.y;
    
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
    
    double load = std::max(0.0, std::min(1.0, node.getCurrentLoad())); // Ensure load is [0, 1]
    double score = 1.0 / (1.0 + std::exp(load * 5));
    
    loadBalanceCache[nodeId] = {load, score};
    std::cout << "Load balance score for Node " << nodeId << ": " << score << "\n";
    return score;
}

// Calculate comprehensive process score
double ContextAwareScheduler::calculateProcessScore(const std::shared_ptr<Process>& process) {
    processScoreCount++;
    int processId = process->getProcessID();
    
    auto cacheIter = processScoreCache.find(processId);
    if (cacheIter != processScoreCache.end()) {
        return cacheIter->second;
    }
    
    double score = (1.0 / (1.0 + std::max(0, process->getPriority()))) + // Priority consideration
                   (0.5 * (1.0 - std::max(0.0, std::min(1.0, process->getMobility())))) + // Mobility factor
                   (0.2 * std::max(0.0, process->getNps())) + // Network Performance Score
                   (0.3 * (1.0 - std::max(0.0, std::min(1.0, process->getRelinquishProbability())))) + // Stability factor
                   (0.4 * std::max(0.0, std::min(1.0, process->getLatencySensitivity()))); // Latency sensitivity
    
    processScoreCache[processId] = score;
    std::cout << "Process score for Process " << processId << ": " << score << "\n";
    return score;
}

// Calculate comprehensive node selection score
double ContextAwareScheduler::calculateNodeScore(const FogNode& node, const std::shared_ptr<Process>& process) {
    nodeScoreCount++;
    int nodeId = node.getNodeID();
    int processId = process->getProcessID();
    
    auto cacheKey = std::make_pair(nodeId, processId);
    auto cacheIter = nodeScoreCache.find(cacheKey);
    if (cacheIter != nodeScoreCache.end()) {
        return cacheIter->second;
    }
    
    double locationScore = calculateLocationScore(process, node);
    double loadBalanceScore = calculateLoadBalanceScore(node);
    
    double delay = std::max(0.0, node.getDelay());
    double bandwidth = std::max(1e-6, node.getBandwidth()); // Avoid division by zero
    double packetLoss = std::max(0.0, std::min(1.0, node.getPacketLoss()));
    double requiredBandwidth = std::max(1e-6, process->getRequiredBandwidth());
    double maxPacketLoss = std::max(1e-6, process->getMaxPacketLoss());
    
    double score = (1.0 / (1.0 + delay)) + // Delay preference
                   (bandwidth / requiredBandwidth) + // Bandwidth utilization
                   (1.0 - (packetLoss / maxPacketLoss)) + // Packet loss
                   locationScore + // Location matching
                   loadBalanceScore; // Load distribution
    
    nodeScoreCache[cacheKey] = score;
    std::cout << "Node score for Node " << nodeId << " and Process " << processId << ": " << score << "\n";
    return score;
}

// Calculate new load after assigning process to node
double ContextAwareScheduler::calculateNewLoad(const FogNode& node, const Resource& resources) {
    double newLoad = node.getCurrentLoad() + (resources.cpu / std::max(1e-6, node.getCpuCapacity()));
    newLoad = std::max(0.0, std::min(1.0, newLoad)); // Ensure load is [0, 1]
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
    resourceCheckCount++;
    int nodeId = node.getNodeID();
    double remainingBandwidth = node.getBandwidth() - nodeUsedBandwidth[nodeId];
    bool fits = (node.getAvailableCpu() >= resources.cpu * 0.5) && // Partial assignment threshold
                node.getAvailableMemory() >= resources.memory && 
                remainingBandwidth >= process.getRequiredBandwidth();
    std::cout << "Resources fit check for Node " << nodeId << " and Process " << process.getProcessID() << ": " << (fits ? "Yes" : "No") << "\n";
    return fits;
}

// Find candidate nodes using quadtree and resource constraints
std::vector<int> ContextAwareScheduler::findCandidateNodes(const std::shared_ptr<Process>& process) {
    std::vector<int> candidates;
    const auto& resources = process->getRequiredResources();
    
    // Define location-to-coordinate mapping for processes
    std::unordered_map<std::string, Point> locationToCoords = {
        {"Zone_A", Point(10.0, 10.0)},
        {"Zone_B", Point(20.0, 10.0)},
        {"Zone_C", Point(10.0, 20.0)},
        {"Zone_D", Point(20.0, 20.0)}
    };

    std::string processLocation = process->getRequestLocation();
    if (locationToCoords.find(processLocation) == locationToCoords.end()) {
        throw std::runtime_error("Unknown process location: " + processLocation + " for Process " + std::to_string(process->getProcessID()));
    }
    Point center = locationToCoords[processLocation];
    const double BASE_RADIUS = 15.0;
    const double ALPHA = 0.5;
    const double MIN_RADIUS = 5.0;
    double radius = std::max(MIN_RADIUS, BASE_RADIUS * (1 - ALPHA * process->getLatencySensitivity()));
    spatialIndex->queryRange(center, radius, candidates, 20);
    
    std::vector<int> finalCandidates;
    for (int nodeId : candidates) {
        const FogNode& node = *nodeMap[nodeId];
        if (node.getCpuCapacity() >= resources.cpu * 0.5 &&
            node.getMemory() >= resources.memory &&
            (node.getBandwidth() - nodeUsedBandwidth[nodeId]) >= process->getRequiredBandwidth()) {
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
    std::vector<std::pair<double, int>> eligibleNodes; // Pair of (overallLoad, nodeId)
    
    // Get candidate nodes for the process
    std::vector<int> candidateNodeIds = findCandidateNodes(process);

    // Evaluate each candidate node
    for (int nodeId : candidateNodeIds) {
        FogNode& node = *nodeMap[nodeId];
        if (!node.getIsActive()) continue;

        auto resources = process->getRequiredResources();
        if (canResourcesFit(node, resources, *process) && canNodeHandleProcess(node, *process)) {
            // Calculate projected CPU load after assignment
            double cpuLoad = node.getCurrentLoad() + (resources.cpu / std::max(1e-6, node.getCpuCapacity()));
            // Calculate projected bandwidth load after assignment
            double bandwidthLoad = (nodeUsedBandwidth[nodeId] + process->getRequiredBandwidth()) / node.getBandwidth();
            // Overall load is the maximum of CPU and bandwidth loads
            double overallLoad = std::max(cpuLoad, bandwidthLoad);
            if (overallLoad <= 1.0) { // Ensure node can handle the process without overloading
                eligibleNodes.emplace_back(overallLoad, nodeId);
            }
        }
    }

    // If there are eligible nodes, select the one with the lowest load
    if (!eligibleNodes.empty()) {
        // Sort by overallLoad in ascending order (lowest load first)
        std::sort(eligibleNodes.begin(), eligibleNodes.end());
        int bestNodeId = eligibleNodes[0].second;
        FogNode& bestNode = *nodeMap[bestNodeId];

        // Attempt to assign the process to the selected node
        if (bestNode.assignProcess(*process)) {
            nodeUsedBandwidth[bestNodeId] += process->getRequiredBandwidth();
            std::cout << "Assigned Process " << process->getProcessID() 
                      << " to Node " << bestNodeId << " with lowest load.\n";
            return bestNodeId;
        }
    }

    // No suitable node found
    std::cout << "No suitable node found for Process " << process->getProcessID() << ".\n";
    return -1;
}

// Partition process across multiple nodes
bool ContextAwareScheduler::partitionProcess(std::shared_ptr<Process> process) {
    partitioningAttemptCount++;
    std::cout << "Attempting to partition Process " << process->getProcessID() << ".\n";
    auto resources = process->getRequiredResources();
    double remainingCpu = resources.cpu;
    double remainingMem = resources.memory;
    double remainingBw = process->getRequiredBandwidth();
    std::vector<int> assignedNodes;
    
    std::priority_queue<std::pair<double, int>> nodeQueue;
    for (const auto& node : fogNodes) {
        if (node.getIsActive() && node.getCurrentLoad() < 1.0 && canNodeHandleProcess(node, *process)) {
            double score = (0.6 * node.getAvailableCpu() / std::max(1e-6, remainingCpu)) + // CPU contribution
                          (0.3 * node.getAvailableMemory() / std::max(1e-6, remainingMem)) + // Memory contribution
                          (0.1 * node.getBandwidth() / std::max(1e-6, remainingBw)); // Bandwidth contribution
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
        
        if (cpuToAssign < MIN_CPU_ALLOCATION) continue;
        
        auto partitionedProcess = std::make_shared<Process>(*process);
        partitionedProcess->setRequiredResources(cpuToAssign, memToAssign);
        
        double remainingBandwidth = node.getBandwidth() - nodeUsedBandwidth[nodeId];
        double bwToAssign = process->getRequiredBandwidth() * (cpuToAssign / resources.cpu);
        if (remainingBandwidth >= bwToAssign) {
            if (node.assignProcess(*partitionedProcess)) {
                nodeUsedBandwidth[nodeId] += process->getRequiredBandwidth();
                remainingBw -= bwToAssign;
                processToNodeMap[process->getProcessID()].push_back(nodeId);
                assignedNodes.push_back(nodeId);
                remainingCpu -= cpuToAssign;
                remainingMem -= memToAssign;
                remainingBw -= bwToAssign;
                updateNodeIndices(nodeId);
                std::cout << "Assigned " << cpuToAssign << " CPU to Node " << nodeId << " for Process " << process->getProcessID() << ".\n";
            }
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
            allProcesses.push_back({process, {}, false, -1.0, -1.0}); // Add as failed
            continue;
        }
        // Increase radius for retries
        auto originalRadius = 100.0 + (10.0 * process->getLatencySensitivity());
        double retryRadius = originalRadius * (1.0 + 0.5 * retryAttempts[processId]);
        std::cout << "Retrying Process " << processId << " with radius " << retryRadius << "\n";
        if (partitionProcess(process)) {
            double start_time = static_cast<double>(process->getArrivalTime());
            double completion_time = start_time + static_cast<double>(process->getBurstTime());
            scheduledProcesses.push_back({process, processToNodeMap[processId]});
            allProcesses.push_back({process, processToNodeMap[processId], true, start_time, completion_time});
            std::cout << "Retry succeeded for Process " << processId << ".\n";
        } else {
            std::cout << "Retry failed for Process " << processId 
                      << " (attempt " << retryAttempts[processId] << ").\n";
            allProcesses.push_back({process, {}, false, -1.0, -1.0}); // Add as failed
        }
    }
    std::cout << "Retry queue processing completed.\n";
}

// Main scheduling method
void ContextAwareScheduler::schedule() {
    std::cout << "Scheduling started. Queue size: " << processPriorityQueue.size() << "\n";
    if (processPriorityQueue.empty()) return;
    
    const size_t MAX_CACHE_SIZE = 1000;
    std::vector<std::shared_ptr<Process>> processes;
    while (!processPriorityQueue.empty()) {
        processes.push_back(getNextProcess());
    }

    // Map to track the earliest available time for each node
    std::unordered_map<int, double> nodeAvailableTime;
    for (const auto& node : fogNodes) {
        nodeAvailableTime[node.getNodeID()] = 0.0; // Initially available at time 0
    }
    
    for (size_t i = 0; i < processes.size(); i++) {
        auto process = processes[i];
        double arrival_time = static_cast<double>(process->getArrivalTime());
        int assignedNode = assignToFogNode(process);
        
        if (assignedNode != -1) {
            FogNode& node = *nodeMap[assignedNode];
            // Start time is the maximum of arrival time and node's available time
            double start_time = std::max(arrival_time, nodeAvailableTime[assignedNode]);
            double completion_time = start_time + static_cast<double>(process->getBurstTime());
            nodeAvailableTime[assignedNode] = completion_time; // Update node's available time
            std::cout << "Process " << process->getProcessID() << " assigned to Node " << assignedNode << ".\n";
            processToNodeMap[process->getProcessID()] = {assignedNode};
            updateNodeIndices(assignedNode);
            scheduledProcesses.push_back({process, {assignedNode}});
            allProcesses.push_back({process, {assignedNode}, true, start_time, completion_time});
        } else if (partitionProcess(process)) {
            // For partitioned processes, use the latest completion time across assigned nodes
            double start_time = arrival_time;
            double latest_completion = start_time;
            for (int nodeId : processToNodeMap[process->getProcessID()]) {
                start_time = std::max(arrival_time, nodeAvailableTime[nodeId]);
                double node_completion = start_time + static_cast<double>(process->getBurstTime()) / processToNodeMap[process->getProcessID()].size(); // Simplified split
                nodeAvailableTime[nodeId] = node_completion;
                latest_completion = std::max(latest_completion, node_completion);
            }
            scheduledProcesses.push_back({process, processToNodeMap[process->getProcessID()]});
            allProcesses.push_back({process, processToNodeMap[process->getProcessID()], true, start_time, latest_completion});
        } else {
            std::cout << "No node found. Adding to retry queue.\n";
            retryQueue.push(process);
            retryAttemptCount++;
            allProcesses.push_back({process, {}, false, -1.0, -1.0}); // Add as failed
        }
        
        // Evict oldest cache entries if size exceeds limit
        if (nodeScoreCache.size() > MAX_CACHE_SIZE) {
            std::vector<std::pair<std::pair<int, int>, double>> entries(nodeScoreCache.begin(), nodeScoreCache.end());
            std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
                return a.second < b.second; // Sort by score (proxy for age)
            });
            size_t evictCount = entries.size() / 5; // Evict 20%
            for (size_t j = 0; j < evictCount; ++j) {
                nodeScoreCache.erase(entries[j].first);
                locationScoreCache.erase(entries[j].first); // Keep caches in sync
            }
            std::cout << "Evicted " << evictCount << " cache entries at iteration " << i << ".\n";
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
        const auto& [process, nodeIds, isScheduled, start_time, completion_time] = allProcesses[i];
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

void ContextAwareScheduler::printSchedulingMetrics() const {
    std::cout << "\n========================================\n";
    std::cout << "       Scheduling Summary Report        \n";
    std::cout << "========================================\n";

    // Per-process timing
    std::cout << "\nPer-Process Timing Details:\n";
    std::cout << "----------------------------------------\n";
    double total_waiting_time = 0.0;
    std::vector<double> waiting_times;
    for (const auto& [process, nodeIds, isScheduled, start_time, completion_time] : allProcesses) {
        std::cout << "Process " << process->getProcessID() << ":\n";
        if (isScheduled) {
            double arrival_time = static_cast<double>(process->getArrivalTime());
            double waiting_time = start_time - arrival_time;
            total_waiting_time += waiting_time;
            waiting_times.push_back(waiting_time);
            std::cout << "  Start Time: " << start_time << " units\n";
            std::cout << "  Completion Time: " << completion_time << " units\n";
            std::cout << "  Waiting Time: " << waiting_time << " units\n";
            std::cout << "  Assigned to Node(s): ";
            for (size_t j = 0; j < nodeIds.size(); ++j) {
                std::cout << nodeIds[j];
                if (j < nodeIds.size() - 1) std::cout << ", ";
            }
            std::cout << "\n";
        } else {
            std::cout << "  Status: Not Scheduled\n";
        }
        std::cout << "----------------------------------------\n";
    }

    // Aggregate metrics
    double min_arrival = std::numeric_limits<double>::max();
    double max_completion = std::numeric_limits<double>::lowest();
    int scheduled_count = 0;
    
    for (const auto& [process, nodeIds, isScheduled, start_time, completion_time] : allProcesses) {
        if (isScheduled) {
            min_arrival = std::min(min_arrival, static_cast<double>(process->getArrivalTime()));
            max_completion = std::max(max_completion, completion_time);
            scheduled_count++;
        }
    }
    
    double total_execution_time = (scheduled_count > 0) ? (max_completion - min_arrival) : 0;
    double avg_waiting_time = (scheduled_count > 0) ? (total_waiting_time / scheduled_count) : 0;
    
    // Throughput
    double throughput = (scheduled_count > 0) ? (static_cast<double>(scheduled_count) / total_execution_time) : 0;
    
    // Resource Utilization
    double total_used_cpu = 0.0;
    double total_cpu_capacity = 0.0;
    double total_used_memory = 0.0;
    double total_memory_capacity = 0.0;
    
    for (const auto& node : fogNodes) {
        if (node.getIsActive()) {
            total_used_cpu += (node.getCpuCapacity() - node.getAvailableCpu());
            total_cpu_capacity += node.getCpuCapacity();
            total_used_memory += (node.getMemory() - node.getAvailableMemory());
            total_memory_capacity += node.getMemory();
        }
    }
    
    double cpu_utilization = (total_cpu_capacity > 0) ? (total_used_cpu / total_cpu_capacity) : 0;
    double memory_utilization = (total_memory_capacity > 0) ? (total_used_memory / total_memory_capacity) : 0;
    
    // Fairness (Variance of waiting times)
    double sum_squared_diff = 0.0;
    for (double wt : waiting_times) {
        sum_squared_diff += (wt - avg_waiting_time) * (wt - avg_waiting_time);
    }
    double variance = (scheduled_count > 0) ? (sum_squared_diff / scheduled_count) : 0;
    
    std::cout << "\nAggregate Metrics:\n";
    std::cout << "----------------------------------------\n";
    std::cout << "Total Execution Time: " << total_execution_time << " units\n";
    std::cout << "Average Waiting Time: " << avg_waiting_time << " units\n";
    std::cout << "Throughput: " << throughput << " processes/unit\n";
    std::cout << "CPU Utilization: " << (cpu_utilization * 100) << "%\n";
    std::cout << "Fairness (Variance of Waiting Times): " << variance << "\n";
    std::cout << "----------------------------------------\n";

    // Calculation counts (Scheduling Overhead)
    int total_calculations = processScoreCount + nodeScoreCount + resourceCheckCount + retryAttemptCount + partitioningAttemptCount;
    std::cout << "\nCalculation Counts:\n";
    std::cout << "----------------------------------------\n";
    std::cout << "Process Scoring: " << processScoreCount << "\n";
    std::cout << "Node Scoring: " << nodeScoreCount << "\n";
    std::cout << "Resource Checks: " << resourceCheckCount << "\n";
    std::cout << "Retry Attempts: " << retryAttemptCount << "\n";
    std::cout << "Partitioning Attempts: " << partitioningAttemptCount << "\n";
    std::cout << "Scheduling Overhead (Total Calculations): " << total_calculations << "\n";
    std::cout << "----------------------------------------\n";

    std::cout << "\nEnd of Scheduling Summary\n";
    std::cout << "========================================\n";
}