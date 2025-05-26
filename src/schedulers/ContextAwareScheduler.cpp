#include "ContextAwareScheduler.h"
#include <glpk.h>
#include <iostream>
#include <cmath>
#include <limits>
#include <queue>
#include <unordered_map>
#include <set>
#include <random>
#include <stdexcept>
#include <memory>

// Add calculation counters as member variables (declared in .h)
int ContextAwareScheduler::processScoreCount = 0;
int ContextAwareScheduler::nodeScoreCount = 0;
int ContextAwareScheduler::resourceCheckCount = 0;
int ContextAwareScheduler::retryAttemptCount = 0;
int ContextAwareScheduler::partitioningAttemptCount = 0;

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
    double score = 1.0 / (1.0 + std::exp(load * 10));
    
    loadBalanceCache[nodeId] = {load, score};
    std::cout << "Load balance score for Node " << nodeId << ": " << score << "\n";
    return score;
}

// Calculate comprehensive process score
double ContextAwareScheduler::calculateProcessScore(const std::shared_ptr<Process>& process) { // process suitability score
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

// Check if a node can handle a process
bool ContextAwareScheduler::canNodeHandleProcess(const FogNode& node, const Process& process) {
    bool canHandle = node.getDelay() <= process.getMaxDelay() && 
                     node.getPacketLoss() <= process.getMaxPacketLoss();
    std::cout << "Node " << node.getNodeID() << " can handle Process " << process.getProcessID() << ": " << (canHandle ? "Yes" : "No") << "\n";
    return canHandle;
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
            resourceCheckCount++;
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
        retryAttemptCount++;
        
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
    std::cout << "Scheduling started with global optimization. Queue size: " << processPriorityQueue.size() << "\n";
    if (processPriorityQueue.empty()) return;

    // Collect all processes from the priority queue
    std::vector<std::shared_ptr<Process>> processes;
    while (!processPriorityQueue.empty()) {
        processes.push_back(getNextProcess()); /**/
    }

    // Create the GLPK problem
    glp_prob *lp = glp_create_prob();
    glp_set_prob_name(lp, "FogScheduling");
    glp_set_obj_dir(lp, GLP_MAX);  // Maximize the objective

    // Define decision variables: x[i][j] is the fraction of process i assigned to node j
    int numVars = processes.size() * fogNodes.size();
    glp_add_cols(lp, numVars);
    for (size_t i = 0; i < processes.size(); ++i) {
        for (size_t j = 0; j < fogNodes.size(); ++j) {
            int varIndex = i * fogNodes.size() + j + 1;  // GLPK indices start at 1
            glp_set_col_name(lp, varIndex, ("x_" + std::to_string(i) + "_" + std::to_string(j)).c_str());
            glp_set_col_bnds(lp, varIndex, GLP_DB, 0.0, 1.0);  // 0 <= x_ij <= 1
        }
    }

    // Objective: Maximize sum of x_ij * s_i * s_j,i
    for (size_t i = 0; i < processes.size(); ++i) {
        double s_i = calculateProcessScore(processes[i]);
        for (size_t j = 0; j < fogNodes.size(); ++j) {
            int varIndex = i * fogNodes.size() + j + 1;
            double s_ji = calculateNodeScore(fogNodes[j], processes[i]);
            double combinedScore = s_i * s_ji;
            glp_set_obj_coef(lp, varIndex, combinedScore);
        }
    }

    // Constraint 1: Each process must be fully assigned (sum of x_ij = 1 for each i)
    glp_add_rows(lp, processes.size());
    for (size_t i = 0; i < processes.size(); ++i) {
        glp_set_row_name(lp, i + 1, ("assign_" + std::to_string(i)).c_str());
        glp_set_row_bnds(lp, i + 1, GLP_FX, 1.0, 1.0);  // sum x_ij = 1
        std::vector<int> indices(fogNodes.size());
        std::vector<double> coeffs(fogNodes.size(), 1.0);
        for (size_t j = 0; j < fogNodes.size(); ++j) {
            indices[j] = i * fogNodes.size() + j + 1;
        }
        glp_set_mat_row(lp, i + 1, fogNodes.size(), indices.data() - 1, coeffs.data() - 1);
    }

    // Constraint 2: Resource constraints for each node
    int resourceRows = fogNodes.size() * 3;  // CPU, memory, bandwidth for each node
    glp_add_rows(lp, resourceRows);
    int rowIndex = processes.size() + 1;
    for (size_t j = 0; j < fogNodes.size(); ++j) {
        const FogNode& node = fogNodes[j];
        // CPU constraint
        glp_set_row_name(lp, rowIndex, ("cpu_" + std::to_string(j)).c_str());
        glp_set_row_bnds(lp, rowIndex, GLP_UP, 0.0, node.getCpuCapacity());
        // Memory constraint
        glp_set_row_name(lp, rowIndex + 1, ("mem_" + std::to_string(j)).c_str());
        glp_set_row_bnds(lp, rowIndex + 1, GLP_UP, 0.0, node.getMemory());
        // Bandwidth constraint
        glp_set_row_name(lp, rowIndex + 2, ("bw_" + std::to_string(j)).c_str());
        glp_set_row_bnds(lp, rowIndex + 2, GLP_UP, 0.0, node.getBandwidth());

        std::vector<int> indices(processes.size());
        std::vector<double> cpuCoeffs(processes.size());
        std::vector<double> memCoeffs(processes.size());
        std::vector<double> bwCoeffs(processes.size());
        for (size_t i = 0; i < processes.size(); ++i) {
            int varIndex = i * fogNodes.size() + j + 1;
            indices[i] = varIndex;
            const auto& resources = processes[i]->getRequiredResources();
            cpuCoeffs[i] = resources.cpu;
            memCoeffs[i] = resources.memory;
            bwCoeffs[i] = processes[i]->getRequiredBandwidth();
        }
        glp_set_mat_row(lp, rowIndex, processes.size(), indices.data() - 1, cpuCoeffs.data() - 1);
        glp_set_mat_row(lp, rowIndex + 1, processes.size(), indices.data() - 1, memCoeffs.data() - 1);
        glp_set_mat_row(lp, rowIndex + 2, processes.size(), indices.data() - 1, bwCoeffs.data() - 1);
        rowIndex += 3;
    }

    // Constraint 3: Latency constraint (x_ij = 0 if node delay > process max delay)
    for (size_t i = 0; i < processes.size(); ++i) {
        for (size_t j = 0; j < fogNodes.size(); ++j) {
            if (fogNodes[j].getDelay() > processes[i]->getMaxDelay()) {
                int varIndex = i * fogNodes.size() + j + 1;
                glp_set_col_bnds(lp, varIndex, GLP_FX, 0.0, 0.0);  // Fix x_ij = 0
            }
        }
    }

    // Solve the problem
    glp_simplex(lp, nullptr);

    if (glp_get_status(lp) == GLP_OPT) {
        std::cout << "Optimal solution found with objective value: " << glp_get_obj_val(lp) << "\n";

        // Process the solution and assign processes to nodes
        std::unordered_map<int, double> nodeAvailableTime;
        for (const auto& node : fogNodes) {
            nodeAvailableTime[node.getNodeID()] = 0.0; //0.0 is the initial time for each node
        }

        for (size_t i = 0; i < processes.size(); ++i) {
            auto process = processes[i];
            double arrival_time = static_cast<double>(process->getArrivalTime());
            std::vector<int> assignedNodes;
            double start_time = arrival_time;
            double latest_completion = start_time;

            for (size_t j = 0; j < fogNodes.size(); ++j) {
                int varIndex = i * fogNodes.size() + j + 1;
                double fraction = glp_get_col_prim(lp, varIndex);
                if (fraction > 0.1) {  // Consider assignments above a small threshold
                    int nodeId = fogNodes[j].getNodeID();
                    FogNode& node = *nodeMap[nodeId];
                    auto resources = process->getRequiredResources();

                    // Create a partitioned process if fraction < 1
                    auto partitionedProcess = std::make_shared<Process>(*process);
                    partitionedProcess->setRequiredResources(resources.cpu * fraction, resources.memory * fraction);
                    partitionedProcess->setRequiredBandwidth(process->getRequiredBandwidth() * fraction);

                    partitioningAttemptCount++;
                    resourceCheckCount++;

                    if (node.assignProcess(*partitionedProcess)) {
                        nodeUsedBandwidth[nodeId] += process->getRequiredBandwidth() * fraction;
                        assignedNodes.push_back(nodeId);

                        // Update timing
                        start_time = std::max(arrival_time, nodeAvailableTime[nodeId]);
                        double completion_time = start_time + static_cast<double>(process->getBurstTime()) * fraction;
                        nodeAvailableTime[nodeId] = completion_time;
                        latest_completion = std::max(latest_completion, completion_time);
                        updateNodeIndices(nodeId);
                    }
                }
            }

            if (!assignedNodes.empty()) { // If assigned to at least one node
                processToNodeMap[process->getProcessID()] = assignedNodes;
                scheduledProcesses.push_back({process, assignedNodes});
                allProcesses.push_back({process, assignedNodes, true, start_time, latest_completion});
                std::cout << "Process " << process->getProcessID() << " assigned to " << assignedNodes.size() << " node(s).\n";
            } else { // If not assigned to any node
                std::cout << "Process " << process->getProcessID() << " could not be assigned.\n";
                allProcesses.push_back({process, {}, false, -1.0, -1.0});
            }
        }
    } else { // If no optimal solution found
        std::cout << "No optimal solution found.\n";
        // Fallback to adding processes to retry queue or marking as failed
        for (auto& process : processes) {
            allProcesses.push_back({process, {}, false, -1.0, -1.0});
            retryQueue.push(process);
            retryAttemptCount++;
        }
        processRetryQueue();
    }

    // Clean up
    glp_delete_prob(lp);

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
    double throughput = (scheduled_count > 0) ? (static_cast<double>(scheduled_count) / total_execution_time) : 0;
    
    std::cout << "\nAggregate Metrics:\n";
    std::cout << "----------------------------------------\n";
    std::cout << "Total Execution Time: " << total_execution_time << " units\n";
    std::cout << "Average Waiting Time: " << avg_waiting_time << " units\n";
    std::cout << "Throughput: " << throughput << " processes/unit-time\n";
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

    std::cout << "\nEnd of Context-Aware Scheduling Summary\n";
    std::cout << "========================================\n";
}