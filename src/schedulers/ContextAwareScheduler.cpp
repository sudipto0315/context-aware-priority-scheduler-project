#include "ContextAwareScheduler.h"
#include <iostream>
#include <cmath>
#include <limits>
#include <algorithm>
#include <queue>
#include <unordered_map>
#include <numeric>
#include <thread>

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

    // Insert the point only in the quadrant that contains it
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

void QuadTree::queryRange(Point center, double radius, std::vector<int>& results, int maxResults) const {
    // Early termination if we've found enough results
    if (maxResults > 0 && static_cast<int>(results.size()) >= maxResults) return;

    // Early rejection test
    if (topLeft.x > center.x + radius || bottomRight.x < center.x - radius ||
        topLeft.y > center.y + radius || bottomRight.y < center.y - radius) {
        return;
    }

    for (const auto& node : nodes) {
        double dx = node.point.x - center.x;
        double dy = node.point.y - center.y;
        if (dx * dx + dy * dy <= radius * radius) {
            results.push_back(node.nodeId);
            // Check early termination after each addition
            if (maxResults > 0 && static_cast<int>(results.size()) >= maxResults) return;
        }
    }

    if (!isLeaf()) {
        // Visit closest quadrants first (optimization)
        double midX = (topLeft.x + bottomRight.x) / 2;
        double midY = (topLeft.y + bottomRight.y) / 2;
        
        // Determine which quadrant the center is in
        bool isWest = center.x < midX;
        bool isNorth = center.y < midY;
        
        // Visit the quadrant containing the center first
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

// ContextAwareScheduler Implementation
ContextAwareScheduler::ContextAwareScheduler(const std::vector<FogNode>& nodes) 
    : fogNodes(nodes), sortedNodes(nodes), nodeMap(), nodeComparator(nodeMap), needsResorting(true) {
    // Initialize direct index mapping for quick lookups
    for (size_t i = 0; i < fogNodes.size(); i++) {
        int nodeId = fogNodes[i].getNodeID();
        nodeMap[nodeId] = &fogNodes[i];
        nodeIndexMap[nodeId] = i;  // Direct index lookup
    }
    
    // Initialize spatial indices with calculated boundaries
    buildSpatialIndices();
    
    // Pre-compute and cache node coordinates
    for (const auto& node : fogNodes) {
        int nodeId = node.getNodeID();
        // Calculate real coordinates or use the simulated ones as before
        double x = (nodeId % 100) * 10.0;
        double y = (nodeId / 100) * 10.0;
        nodeCoordinates[nodeId] = {x, y};
    }
}

void ContextAwareScheduler::buildSpatialIndices() {
    // Calculate actual boundaries rather than assuming fixed size
    double minX = std::numeric_limits<double>::max();
    double minY = std::numeric_limits<double>::max();
    double maxX = std::numeric_limits<double>::lowest();
    double maxY = std::numeric_limits<double>::lowest();
    
    for (const auto& node : fogNodes) {
        int nodeId = node.getNodeID();
        double x = (nodeId % 100) * 10.0;
        double y = (nodeId / 100) * 10.0;
        
        minX = std::min(minX, x);
        minY = std::min(minY, y);
        maxX = std::max(maxX, x);
        maxY = std::max(maxY, y);
    }
    
    // Add a small buffer
    minX -= 10.0;
    minY -= 10.0;
    maxX += 10.0;
    maxY += 10.0;
    
    nodesByCapacity.clear();
    nodesByMemory.clear();
    nodesByBandwidth.clear();
    
    // Initialize quadtree with calculated boundaries
    spatialIndex = std::make_unique<QuadTree>(Point(minX, minY), Point(maxX, maxY));
    
    // Build multi-dimensional indices
    for (size_t i = 0; i < fogNodes.size(); i++) {
        const FogNode& node = fogNodes[i];
        int nodeId = node.getNodeID();
        
        // Calculate real coordinates or use the simulated ones
        double x = (nodeId % 100) * 10.0;
        double y = (nodeId / 100) * 10.0;
        
        spatialIndex->insert(x, y, nodeId);
        nodesByCapacity[node.getCpuCapacity()].push_back(nodeId);
        nodesByMemory[node.getMemory()].push_back(nodeId);
        nodesByBandwidth[node.getBandwidth()].push_back(nodeId);
    }
    
    // Sort resource vectors for binary search
    for (auto& entry : nodesByCapacity) {
        std::sort(entry.second.begin(), entry.second.end());
    }
    for (auto& entry : nodesByMemory) {
        std::sort(entry.second.begin(), entry.second.end());
    }
    for (auto& entry : nodesByBandwidth) {
        std::sort(entry.second.begin(), entry.second.end());
    }
}

void ContextAwareScheduler::updateNodeIndices(int nodeId) {
    // Use direct lookup instead of find_if
    if (nodeIndexMap.find(nodeId) == nodeIndexMap.end()) return;
    
    const FogNode& node = *nodeMap[nodeId];
    
    // Since we can't easily erase by the first element of the composite key,
    // just clear the cache if it gets too large
    if (nodeScoreCache.size() > 1000) {
        nodeScoreCache.clear();
        locationScoreCache.clear();
        loadBalanceCache.erase(nodeId); // This one uses nodeId directly as key
    }
    
    // Update resource indices
    for (auto& entry : nodesByCapacity) {
        auto& nodeList = entry.second;
        nodeList.erase(std::remove(nodeList.begin(), nodeList.end(), nodeId), nodeList.end());
    }
    for (auto& entry : nodesByMemory) {
        auto& nodeList = entry.second;
        nodeList.erase(std::remove(nodeList.begin(), nodeList.end(), nodeId), nodeList.end());
    }
    for (auto& entry : nodesByBandwidth) {
        auto& nodeList = entry.second;
        nodeList.erase(std::remove(nodeList.begin(), nodeList.end(), nodeId), nodeList.end());
    }
    
    // Add to appropriate capacity buckets
    double availableCapacity = node.getCpuCapacity() * (1.0 - node.getCurrentLoad());
    nodesByCapacity[availableCapacity].push_back(nodeId);
    nodesByMemory[node.getAvailableMemory()].push_back(nodeId);
    nodesByBandwidth[node.getBandwidth()].push_back(nodeId);
    
    // Flag for resorting
    needsResorting = true;
}

void ContextAwareScheduler::addProcess(std::shared_ptr<Process> process) {
    BaseScheduler::addProcess(process);
    double score = calculateProcessScore(process);
    process->setProcessScore(score);
    
    // Group similar processes
    int priority = process->getPriority();
    double latencySensitivity = process->getLatencySensitivity();
    double resourceNeeds = process->getRequiredResources().cpu + process->getRequiredResources().memory;
    
    // Create a simple hash for process grouping
    int groupHash = static_cast<int>((priority * 100) + (latencySensitivity * 10) + (resourceNeeds / 10));
    processesByGroup[groupHash].push_back(process);
    
    processPriorityQueue.push(process);
    std::cout << "Process " << process->getProcessID() << " added to queue with score " << score 
              << " (group: " << groupHash << ").\n";
}

double ContextAwareScheduler::calculateLocationScore(const std::shared_ptr<Process>& process, const FogNode& node) {
    int processId = process->getProcessID();
    int nodeId = node.getNodeID();
    
    // Check if we've already calculated this score
    auto cacheKey = std::make_pair(processId, nodeId);
    auto cacheIter = locationScoreCache.find(cacheKey);
    if (cacheIter != locationScoreCache.end()) {
        return cacheIter->second;
    }
    
    // Use pre-cached coordinates
    double px = (processId % 100) * 10.0;
    double py = (processId / 100) * 10.0;
    
    const auto& nodeCoord = nodeCoordinates[nodeId];
    double nx = nodeCoord.first;
    double ny = nodeCoord.second;
    
    double distance = std::sqrt((px - nx) * (px - nx) + (py - ny) * (py - ny));
    double score = 1.0 / (1.0 + distance);
    
    // Cache the result
    locationScoreCache[cacheKey] = score;
    return score;
}

double ContextAwareScheduler::calculateLoadBalanceScore(const FogNode& node) {
    int nodeId = node.getNodeID();
    
    // Check cache
    auto cacheIter = loadBalanceCache.find(nodeId);
    if (cacheIter != loadBalanceCache.end() && 
        std::abs(cacheIter->second.first - node.getCurrentLoad()) < 0.01) {
        return cacheIter->second.second;
    }
    
    double score = 1.0 / (1.0 + std::exp(std::min(node.getCurrentLoad() * 5, 10.0)));
    
    // Cache the result with the load that generated it
    loadBalanceCache[nodeId] = {node.getCurrentLoad(), score};
    return score;
}

double ContextAwareScheduler::calculateProcessScore(const std::shared_ptr<Process>& process) {
    int processId = process->getProcessID();
    
    // Check cache
    auto cacheIter = processScoreCache.find(processId);
    if (cacheIter != processScoreCache.end()) {
        return cacheIter->second;
    }
    
    double score = (1.0 / (1.0 + process->getPriority())) + 
                   (0.5 * (1.0 - process->getMobility())) + 
                   (0.2 * process->getNps()) + 
                   (0.3 * (1.0 - process->getRelinquishProbability())) + 
                   (0.4 * process->getLatencySensitivity());
    
    // Cache the result
    processScoreCache[processId] = score;
    return score;
}

double ContextAwareScheduler::calculateNodeScore(const FogNode& node, const std::shared_ptr<Process>& process) {
    int nodeId = node.getNodeID();
    int processId = process->getProcessID();
    
    // Check cache for combined score
    auto cacheKey = std::make_pair(nodeId, processId);
    auto cacheIter = nodeScoreCache.find(cacheKey);
    if (cacheIter != nodeScoreCache.end()) {
        return cacheIter->second;
    }
    
    double locationScore = calculateLocationScore(process, node);
    double loadBalanceScore = calculateLoadBalanceScore(node);
    
    double score = (1.0 / (1.0 + node.getDelay())) + 
                   (node.getBandwidth() / process->getRequiredBandwidth()) + 
                   (1.0 - (node.getPacketLoss() / process->getMaxPacketLoss())) + 
                   locationScore + 
                   loadBalanceScore;
    
    // Cache the result
    nodeScoreCache[cacheKey] = score;
    return score;
}

double ContextAwareScheduler::calculateNewLoad(const FogNode& node, const Resource& resources) {
    return node.getCurrentLoad() + (resources.cpu / node.getCpuCapacity());
}

bool ContextAwareScheduler::canNodeHandleProcess(const FogNode& node, const Process& process) {
    return node.getDelay() <= process.getMaxDelay() && 
           node.getPacketLoss() <= process.getMaxPacketLoss();
}

bool ContextAwareScheduler::canResourcesFit(const FogNode& node, const Resource& resources, const Process& process) {
    return (node.getCpuCapacity() >= resources.cpu * 0.5) && 
           node.getMemory() >= resources.memory && 
           node.getBandwidth() >= process.getRequiredBandwidth();
}

std::vector<int> ContextAwareScheduler::findCandidateNodes(const std::shared_ptr<Process>& process) {
    std::vector<int> candidates;
    
    // Get the process requirements
    const auto& resources = process->getRequiredResources();
    
    // First filter by capacity requirements (multi-dimensional filtering)
    std::vector<int> cpuCandidates;
    std::vector<int> memCandidates;
    std::vector<int> bwCandidates;
    
    // Find nodes with sufficient CPU capacity
    for (const auto& entry : nodesByCapacity) {
        if (entry.first >= resources.cpu * 0.5) {
            cpuCandidates.insert(cpuCandidates.end(), entry.second.begin(), entry.second.end());
        }
    }
    
    // Find nodes with sufficient memory
    for (const auto& entry : nodesByMemory) {
        if (entry.first >= resources.memory) {
            memCandidates.insert(memCandidates.end(), entry.second.begin(), entry.second.end());
        }
    }
    
    // Find nodes with sufficient bandwidth
    for (const auto& entry : nodesByBandwidth) {
        if (entry.first >= process->getRequiredBandwidth()) {
            bwCandidates.insert(bwCandidates.end(), entry.second.begin(), entry.second.end());
        }
    }
    
    // Early termination if any resource type has no candidates
    if (cpuCandidates.empty() || memCandidates.empty() || bwCandidates.empty()) {
        return candidates;  // Empty result
    }
    
    // Sort for set intersection
    std::sort(cpuCandidates.begin(), cpuCandidates.end());
    std::sort(memCandidates.begin(), memCandidates.end());
    std::sort(bwCandidates.begin(), bwCandidates.end());
    
    // Intersect to find nodes that satisfy all resource requirements
    std::vector<int> resourceCandidates;
    
    std::set_intersection(
        cpuCandidates.begin(), cpuCandidates.end(),
        memCandidates.begin(), memCandidates.end(),
        std::back_inserter(resourceCandidates)
    );
    
    std::vector<int> finalResourceCandidates;
    
    std::set_intersection(
        resourceCandidates.begin(), resourceCandidates.end(),
        bwCandidates.begin(), bwCandidates.end(),
        std::back_inserter(finalResourceCandidates)
    );
    
    // If no candidates after resource filtering, return empty
    if (finalResourceCandidates.empty()) {
        return candidates;
    }
    
    // Now filter by location using quadtree
    // We'll use process priority to adjust search radius - higher priority gets wider search
    double px = (process->getProcessID() % 100) * 10.0;
    double py = (process->getProcessID() / 100) * 10.0;
    double radius = 100.0 + (10.0 * process->getLatencySensitivity());
    
    // Limit max results to improve performance for large datasets
    int maxResults = 20; // Adjustable based on your needs
    spatialIndex->queryRange(Point(px, py), radius, candidates, maxResults);
    
    // Intersect spatial candidates with resource candidates
    std::vector<int> finalCandidates;
    
    std::sort(candidates.begin(), candidates.end());
    
    std::set_intersection(
        candidates.begin(), candidates.end(),
        finalResourceCandidates.begin(), finalResourceCandidates.end(),
        std::back_inserter(finalCandidates)
    );
    
    return finalCandidates;
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
    
    // Get candidate nodes efficiently
    std::vector<int> candidateNodeIds = findCandidateNodes(process);
    
    // If we have too many candidates, limit to top N by pre-sorting
    const size_t MAX_CANDIDATES = 10;
    if (candidateNodeIds.size() > MAX_CANDIDATES) {
        // Create pairs of (nodeId, rough estimate score) for pre-sorting
        std::vector<std::pair<int, double>> scoredCandidates;
        
        for (int nodeId : candidateNodeIds) {
            const FogNode& node = *nodeMap[nodeId];
            if (!node.getIsActive()) continue;
            
            // Quick rough score estimate based on load
            double roughScore = 1.0 - node.getCurrentLoad();
            scoredCandidates.emplace_back(nodeId, roughScore);
        }
        
        // Sort by rough score and keep top MAX_CANDIDATES
        std::partial_sort(
            scoredCandidates.begin(), 
            scoredCandidates.begin() + std::min(MAX_CANDIDATES, scoredCandidates.size()), 
            scoredCandidates.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; }
        );
        
        // Replace candidateNodeIds with the pruned list
        candidateNodeIds.clear();
        for (size_t i = 0; i < std::min(MAX_CANDIDATES, scoredCandidates.size()); i++) {
            candidateNodeIds.push_back(scoredCandidates[i].first);
        }
    }
    
    // Now evaluate the pruned candidates with full scoring
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
                    
                    // Early termination if we find a really good match (>80% of theoretical max)
                    if (score > 4.0) {  // Adjust threshold based on your scoring scale
                        break;
                    }
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
    
    // Greedy approach: sort nodes by available capacity to maximize efficient use
    std::vector<std::pair<int, double>> nodesByAvailableCapacity;
    
    for (const auto& node : fogNodes) {
        if (node.getIsActive() && node.getCurrentLoad() < 1.0 && canNodeHandleProcess(node, *process)) {
            // Calculate a combined resource availability score
            double cpuAvail = node.getAvailableCpu();
            double memAvail = node.getAvailableMemory();
            double bwAvail = node.getBandwidth();
            
            // Skip nodes that can't handle any part of the process
            if (cpuAvail <= 0 || memAvail <= 0 || bwAvail <= 0) continue;
            
            // Calculate weighted capacity score (prioritize CPU)
            double capacityScore = (0.6 * cpuAvail / remainingCpu) + 
                                  (0.3 * memAvail / remainingMem) + 
                                  (0.1 * bwAvail / remainingBw);
            
            nodesByAvailableCapacity.emplace_back(node.getNodeID(), capacityScore);
        }
    }
    
    // Sort by capacity score (greedy allocation)
    std::sort(nodesByAvailableCapacity.begin(), nodesByAvailableCapacity.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Batch allocation - try to assign larger chunks to each node
    const double MIN_CPU_ALLOCATION = 0.1;  // Avoid excessive fragmentation
    
    for (const auto& entry : nodesByAvailableCapacity) {
        int nodeId = entry.first;
        FogNode& node = *nodeMap[nodeId];
        
        double cpuAvailable = node.getAvailableCpu();
        double memAvailable = node.getAvailableMemory();
        double bwAvailable = node.getBandwidth();
        
        // Determine optimal resource allocation
        double cpuProportion = std::min(1.0, cpuAvailable / remainingCpu);
        double memProportion = std::min(1.0, memAvailable / remainingMem);
        double bwProportion = std::min(1.0, bwAvailable / remainingBw);
        
        // Take the minimum proportion to ensure balanced allocation
        double proportion = std::min({cpuProportion, memProportion, bwProportion});
        
        // Ensure we allocate a meaningful amount
        if (proportion * remainingCpu < MIN_CPU_ALLOCATION) {
            proportion = std::min(1.0, MIN_CPU_ALLOCATION / remainingCpu);
        }
        
        // Calculate resources to assign
        double cpuToAssign = proportion * remainingCpu;
        double memToAssign = proportion * remainingMem;
        double bwToAssign = proportion * remainingBw;
        
        // Skip if allocation would be too small
        if (cpuToAssign < MIN_CPU_ALLOCATION) continue;
        
        // Create a partitioned process and assign
        auto partitionedProcess = std::make_shared<Process>(*process);
        partitionedProcess->setRequiredResources(cpuToAssign, memToAssign);
        
        if (node.assignProcess(*partitionedProcess)) {
            processToNodeMap[process->getProcessID()].push_back(nodeId);
            assignedNodes.push_back(nodeId);
            
            remainingCpu -= cpuToAssign;
            remainingMem -= memToAssign;
            remainingBw -= bwToAssign;
            
            updateNodeIndices(nodeId);
            
            // Check if we've allocated everything
            if (remainingCpu <= 0.001 && remainingMem <= 0.001 && remainingBw <= 0.001) {
                break;
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

void ContextAwareScheduler::processRetryQueue() {
    // Limit number of retry attempts
    const int MAX_RETRY_ATTEMPTS = 3;
    std::unordered_map<int, int> retryAttempts;
    
    while (!retryQueue.empty()) {
        auto process = retryQueue.top();
        retryQueue.pop();
        
        int processId = process->getProcessID();
        retryAttempts[processId]++;
        
        if (retryAttempts[processId] > MAX_RETRY_ATTEMPTS) {
            std::cout << "Max retry attempts reached for Process " << processId << ".\n";
            continue;
        }
        
        if (!partitionProcess(process)) {
            std::cout << "Retry failed for Process " << processId 
                      << " (attempt " << retryAttempts[processId] << ").\n";
        } else {
            std::cout << "Retry succeeded for Process " << processId << ".\n";
        }
    }
}

void ContextAwareScheduler::schedule() {
    std::cout << "Scheduling started. Queue size: " << processPriorityQueue.size() << "\n";
    if (processPriorityQueue.empty()) return;
    
    // Only sort when needed (optimization)
    if (needsResorting) {
        sortedNodes = fogNodes;
        std::sort(sortedNodes.begin(), sortedNodes.end(), 
                 [](const FogNode& a, const FogNode& b) { return a.getCurrentLoad() < b.getCurrentLoad(); });
        needsResorting = false;
    }
    
    // Process by groups for better locality and cache efficiency
    std::vector<std::shared_ptr<Process>> groupedProcesses;
    
    // Extract all processes while preserving priority order
    while (!processPriorityQueue.empty()) {
        groupedProcesses.push_back(getNextProcess());
    }
    
    // Group processes with similar requirements
    std::sort(groupedProcesses.begin(), groupedProcesses.end(), 
             [this](const auto& a, const auto& b) {
                 int groupA = static_cast<int>((a->getPriority() * 100) + 
                                              (a->getLatencySensitivity() * 10) + 
                                              (a->getRequiredResources().cpu / 10));
                 int groupB = static_cast<int>((b->getPriority() * 100) + 
                                              (b->getLatencySensitivity() * 10) + 
                                              (b->getRequiredResources().cpu / 10));
                 
                 if (groupA != groupB) return groupA < groupB;
                 return a->getProcessScore() > b->getProcessScore();
             });
    
    // Batch processing of similar processes improves cache locality
    for (size_t i = 0; i < groupedProcesses.size(); i++) {
        auto process = groupedProcesses[i];
        
        // Try to assign to a single node first
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
        
        // Clear caches periodically to prevent memory growth
        if (i % 100 == 0) {
            // Keep the most recent entries (e.g., 20% of cache)
            if (nodeScoreCache.size() > 1000) {
                // Simple approach: just clear everything
                // A more sophisticated approach would be to keep most frequently used entries
                nodeScoreCache.clear();
                locationScoreCache.clear();
            }
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