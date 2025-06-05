#include "ContextAwareScheduler.h"
#include <glpk.h>
#include <iostream>
#include <cmath>
#include <climits>
#include <sstream>
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
    double activityLevel = calculateUserActivityLevel(process->getUsageHistory());
    process->setUserActivityLevel(activityLevel);
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
              << ", userActivityLevel: " << activityLevel << " (, groupHash: " << groupHash << ").\n";
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

double ContextAwareScheduler::calculateUserActivityLevel(const std::string& usageHistory) {
    // Parse comma-separated values
    std::vector<double> activityValues;
    std::stringstream ss(usageHistory);
    std::string value;
    while (std::getline(ss, value, ',')) {
        try {
            activityValues.push_back(std::stod(value));
        } catch (const std::exception& e) {
            std::cerr << "Error parsing usageHistory: " << usageHistory << "\n";
            return 0.0; // Default for invalid format
        }
    }

    // Ensure we have exactly 3 values
    if (activityValues.size() != 3) {
        std::cerr << "Invalid usageHistory format: " << usageHistory << "\n";
        return 0.0; // Default for incorrect number of values
    }

    double weightedAverage = (activityValues[0] + activityValues[1] + activityValues[2]) / 3.0;
    double activityLevel = weightedAverage * 10.0; // Scale to 0–10
    return std::max(0.0, std::min(10.0, activityLevel)); // Clamp to [0, 10]
}

// Calculate comprehensive process score
double ContextAwareScheduler::calculateProcessScore(const std::shared_ptr<Process>& process) { // process suitability score
    processScoreCount++;
    int processId = process->getProcessID();
    
    auto cacheIter = processScoreCache.find(processId);
    if (cacheIter != processScoreCache.end()) {
        return cacheIter->second;
    }

    double userActivityLevel = calculateUserActivityLevel(process->getUsageHistory());
    double score = (1.0 / (1.0 + std::max(0, process->getPriority()))) + // Priority consideration
                   (0.5 * (1.0 - std::max(0.0, std::min(1.0, process->getMobility())))) + // Mobility factor
                   (0.2 * userActivityLevel / 10) + // User activity level (normalized)
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
    while (!nodeQueue.empty() && remainingCpu > 0.001 && remainingMem > 0.001 && remainingBw > 0.001) {
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
                if (fraction > 0.001) {  // Consider assignments above a small threshold
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
    std::cout << "\n=== Context-Aware Scheduled Processes Summary ===\n";
    std::cout << "Process_ID,Arrival_Time,Burst_Time,Start_Time,Completion_Time,Waiting_Time,Turnaround_Time,Assigned_Nodes,Status\n";
    
    for (const auto& [process, nodeIds, isScheduled, start_time, completion_time] : allProcesses) {
        std::cout << process->getProcessID() << ","
                  << process->getArrivalTime() << ","
                  << process->getBurstTime() << ",";
        
        if (isScheduled) {
            double arrival_time = static_cast<double>(process->getArrivalTime());
            double waiting_time = start_time - arrival_time;
            double turnaround_time = completion_time - arrival_time;
            
            std::cout << start_time << ","
                      << completion_time << ","
                      << waiting_time << ","
                      << turnaround_time << ",";
            
            // Print assigned nodes
            std::cout << "\"";
            for (size_t j = 0; j < nodeIds.size(); ++j) {
                std::cout << nodeIds[j];
                if (j < nodeIds.size() - 1) std::cout << ";";
            }
            std::cout << "\",Scheduled\n";
        } else {
            std::cout << "-1,-1,-1,-1,\"\",Failed\n";
        }
    }
    std::cout << "==================================================\n";
}

// Print scheduling metrics
void ContextAwareScheduler::printSchedulingMetrics() const {
    std::cout << "\n========================================\n";
    std::cout << "    Context-Aware Scheduling Metrics    \n";
    std::cout << "========================================\n";

    // Calculate basic metrics
    double total_waiting_time = 0.0;
    double total_turnaround_time = 0.0;
    double min_arrival = std::numeric_limits<double>::max();
    double max_completion = std::numeric_limits<double>::lowest();
    int scheduled_count = 0;
    int total_processes = allProcesses.size();
    
    // Resource utilization tracking
    std::unordered_map<int, double> node_utilization_time;
    double total_cpu_used = 0.0;
    double total_memory_used = 0.0;
    double total_bandwidth_used = 0.0;
    double total_cpu_capacity = 0.0;
    double total_memory_capacity = 0.0;
    double total_bandwidth_capacity = 0.0;

    // Initialize node capacities
    for (const auto& node : fogNodes) {
        total_cpu_capacity += node.getCpuCapacity();
        total_memory_capacity += node.getMemory();
        total_bandwidth_capacity += node.getBandwidth();
        node_utilization_time[node.getNodeID()] = 0.0;
    }

    // Process metrics calculation
    for (const auto& [process, nodeIds, isScheduled, start_time, completion_time] : allProcesses) {
        if (isScheduled) {
            double arrival_time = static_cast<double>(process->getArrivalTime());
            double waiting_time = start_time - arrival_time;
            double turnaround_time = completion_time - arrival_time;
            
            total_waiting_time += waiting_time;
            total_turnaround_time += turnaround_time;
            min_arrival = std::min(min_arrival, arrival_time);
            max_completion = std::max(max_completion, completion_time);
            scheduled_count++;
            
            // Resource usage calculation
            auto resources = process->getRequiredResources();
            double process_duration = static_cast<double>(process->getBurstTime());
            
            // For partitioned processes, distribute resources across nodes
            double cpu_per_node = resources.cpu / nodeIds.size();
            double memory_per_node = resources.memory / nodeIds.size();
            double bandwidth_per_node = process->getRequiredBandwidth() / nodeIds.size();
            
            for (int nodeId : nodeIds) {
                node_utilization_time[nodeId] += process_duration;
                total_cpu_used += cpu_per_node * process_duration;
                total_memory_used += memory_per_node * process_duration;
                total_bandwidth_used += bandwidth_per_node * process_duration;
            }
        }
    }

    // Calculate derived metrics
    double total_execution_time = (scheduled_count > 0) ? (max_completion - min_arrival) : 0;
    double avg_waiting_time = (scheduled_count > 0) ? (total_waiting_time / scheduled_count) : 0;
    double avg_turnaround_time = (scheduled_count > 0) ? (total_turnaround_time / scheduled_count) : 0;
    double throughput = (total_execution_time > 0) ? (static_cast<double>(scheduled_count) / total_execution_time) : 0;
    double success_rate = (total_processes > 0) ? (static_cast<double>(scheduled_count) / total_processes * 100.0) : 0;
    
    // Resource utilization percentages
    double cpu_utilization = (total_cpu_capacity * total_execution_time > 0) ? 
                            (total_cpu_used / (total_cpu_capacity * total_execution_time) * 100.0) : 0;
    double memory_utilization = (total_memory_capacity * total_execution_time > 0) ? 
                               (total_memory_used / (total_memory_capacity * total_execution_time) * 100.0) : 0;
    double bandwidth_utilization = (total_bandwidth_capacity * total_execution_time > 0) ? 
                                  (total_bandwidth_used / (total_bandwidth_capacity * total_execution_time) * 100.0) : 0;

    // Scheduling overhead
    int total_calculations = processScoreCount + nodeScoreCount + resourceCheckCount + 
                           retryAttemptCount + partitioningAttemptCount;

    // Print summary metrics in CSV format for easy plotting
    std::cout << "\n=== DETAILED SUMMARY METRICS (CSV FORMAT) ===\n";
    std::cout << "Metric,Value\n";
    std::cout << "Algorithm,Context-Aware\n";
    std::cout << "Total Processes," << total_processes << "\n";
    std::cout << "Scheduled Processes," << scheduled_count << "\n";
    std::cout << "Failed Processes," << (total_processes - scheduled_count) << "\n";
    std::cout << "Success Rate," << success_rate << "%\n";
    std::cout << "Total Execution Time," << total_execution_time << " units\n";
    std::cout << "Average Waiting Time," << avg_waiting_time << " units\n";
    std::cout << "Average Turnaround Time," << avg_turnaround_time << " units\n";
    std::cout << "Throughput," << throughput << " processes/unit\n";
    std::cout << "CPU Utilization," << cpu_utilization << "%\n";
    std::cout << "Memory Utilization," << memory_utilization << "%\n";
    std::cout << "Bandwidth Utilization," << bandwidth_utilization << "%\n";
    std::cout << "Scheduling Overhead," << total_calculations << " calculations\n";
    std::cout << "Process Score Calculations," << processScoreCount << "\n";
    std::cout << "Node Score Calculations," << nodeScoreCount << "\n";
    std::cout << "Resource Checks," << resourceCheckCount << "\n";
    std::cout << "Retry Attempts," << retryAttemptCount << "\n";
    std::cout << "Partitioning Attempts," << partitioningAttemptCount << "\n";

    std::cout << "\n=== NODE UTILIZATION ===\n";
    std::cout << "Node_ID,Utilization_Time,Utilization_Percent\n";
    for (const auto& [nodeId, util_time] : node_utilization_time) {
        double util_percent = (total_execution_time > 0) ? (util_time / total_execution_time * 100.0) : 0;
        std::cout << nodeId << "," << util_time << "," << util_percent << "\n";
    }

    std::cout << "\nEnd of Context-Aware Scheduling Analysis\n";
    std::cout << "========================================\n";
}