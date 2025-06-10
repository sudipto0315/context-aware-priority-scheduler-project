#include "ContextAwareScheduler.h"
#include <iostream>
#include <cmath>
#include <climits>
#include <sstream>
#include <algorithm>
// Greedy heuristic is we will find a feasible solution but not necessarily optimal.
// Static member initialization
int ContextAwareScheduler::processScoreCount = 0;
int ContextAwareScheduler::nodeScoreCount = 0;
int ContextAwareScheduler::resourceCheckCount = 0;
int ContextAwareScheduler::retryAttemptCount = 0;
int ContextAwareScheduler::partitioningAttemptCount = 0;

// Constructor
ContextAwareScheduler::ContextAwareScheduler(const std::vector<FogNode>& nodes) 
    : fogNodes(nodes), sortedNodes(nodes), nodeMap(), nodeComparator(nodeMap), needsResorting(false) {
    for (size_t i = 0; i < fogNodes.size(); i++) {
        int nodeId = fogNodes[i].getNodeID();
        nodeMap[nodeId] = &fogNodes[i];
        nodeIndexMap[nodeId] = i;
        nodeUsedBandwidth[nodeId] = 0.0;
    }
}

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

// Utility methods (unchanged)
void ContextAwareScheduler::updateNodeIndices(int nodeId) {
    if (nodeIndexMap.find(nodeId) == nodeIndexMap.end()) return;
    
    const FogNode& node = *nodeMap[nodeId];
    double oldCapacity = node.getCpuCapacity();
    double availableCapacity = node.getCpuCapacity() * (1.0 - node.getCurrentLoad());
    
    nodesByCapacity.erase({oldCapacity, nodeId});
    nodesByMemory.erase({node.getMemory(), nodeId});
    nodesByBandwidth.erase({node.getBandwidth(), nodeId});
    
    nodesByCapacity.emplace(availableCapacity, nodeId);
    nodesByMemory.emplace(node.getAvailableMemory(), nodeId);
    nodesByBandwidth.emplace(node.getBandwidth(), nodeId);
    std::cout << "Node " << nodeId << " indices updated. New capacity: " << availableCapacity << "\n";
}

double ContextAwareScheduler::calculateLocationScore(const std::shared_ptr<Process>& process, const FogNode& node) {
    int processId = process->getProcessID();
    int nodeId = node.getNodeID();
    
    auto cacheKey = std::make_pair(processId, nodeId);
    auto cacheIter = locationScoreCache.find(cacheKey);
    if (cacheIter != locationScoreCache.end()) {
        return cacheIter->second;
    }

    // Simple 2D Point structure
    struct Point {
        double x, y;
        Point(double x_ = 0, double y_ = 0) : x(x_), y(y_) {}
    };

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
    
    double distance = std::sqrt((px - nx) * (px - nx) + (py - ny) * (py - ny)); // Euclidean distance of process location to node location
    double score = 1.0 / (1.0 + distance);
    
    locationScoreCache[cacheKey] = score;
    std::cout << "Location score for Process " << processId << " and Node " << nodeId << ": " << score << "\n";
    return score;
}

double ContextAwareScheduler::calculateLoadBalanceScore(const FogNode& node) {
    int nodeId = node.getNodeID();
    
    auto cacheIter = loadBalanceCache.find(nodeId);
    if (cacheIter != loadBalanceCache.end() && 
        std::abs(cacheIter->second.first - node.getCurrentLoad()) < 0.01) {
        return cacheIter->second.second;
    }
    
    double load = std::max(0.0, std::min(1.0, node.getCurrentLoad()));
    double score = 1.0 / (1.0 + std::exp(load * 10)); // Exponential decay function for load balance score
    
    loadBalanceCache[nodeId] = {load, score};
    std::cout << "Load balance score for Node " << nodeId << ": " << score << "\n";
    return score;
}

double ContextAwareScheduler::calculateUserActivityLevel(const std::string& usageHistory) {
    std::vector<double> activityValues;
    std::stringstream ss(usageHistory);
    std::string value;
    while (std::getline(ss, value, ',')) {
        try {
            activityValues.push_back(std::stod(value));
        } catch (const std::exception& e) {
            std::cerr << "Error parsing usageHistory: " << usageHistory << "\n";
            return 0.0;
        }
    }

    if (activityValues.size() != 3) {
        std::cerr << "Invalid usageHistory format: " << usageHistory << "\n";
        return 0.0;
    }

    double weightedAverage = (activityValues[0] + activityValues[1] + activityValues[2]) / 3.0;
    double activityLevel = weightedAverage * 10.0;
    return std::max(0.0, std::min(10.0, activityLevel));
}

double ContextAwareScheduler::calculateProcessScore(const std::shared_ptr<Process>& process) {
    processScoreCount++;
    int processId = process->getProcessID();
    
    auto cacheIter = processScoreCache.find(processId);
    if (cacheIter != processScoreCache.end()) {
        return cacheIter->second;
    }

    double userActivityLevel = calculateUserActivityLevel(process->getUsageHistory());
    double score = (1.0 / (1.0 + std::max(0, process->getPriority()))) +
                   (0.5 * (1.0 - std::max(0.0, std::min(1.0, process->getMobility())))) +
                   (0.2 * userActivityLevel / 10) +
                   (0.3 * (1.0 - std::max(0.0, std::min(1.0, process->getRelinquishProbability())))) +
                   (0.4 * std::max(0.0, std::min(1.0, process->getLatencySensitivity())));
    
    processScoreCache[processId] = score;
    std::cout << "Process score for Process " << processId << ": " << score << "\n";
    return score;
}

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
    double bandwidth = std::max(1e-6, node.getBandwidth());
    double packetLoss = std::max(0.0, std::min(1.0, node.getPacketLoss()));
    double requiredBandwidth = std::max(1e-6, process->getRequiredBandwidth());
    double maxPacketLoss = std::max(1e-6, process->getMaxPacketLoss());
    
    double score = (1.0 / (1.0 + delay)) +
                   (bandwidth / requiredBandwidth) +
                   (1.0 - (packetLoss / maxPacketLoss)) +
                   locationScore +
                   loadBalanceScore;
    
    nodeScoreCache[cacheKey] = score;
    std::cout << "Node score for Node " << nodeId << " and Process " << processId << ": " << score << "\n";
    return score;
}

bool ContextAwareScheduler::canNodeHandleProcess(const FogNode& node, const Process& process) {
    bool canHandle = node.getDelay() <= process.getMaxDelay() && 
                     node.getPacketLoss() <= process.getMaxPacketLoss();
    std::cout << "Node " << node.getNodeID() << " can handle Process " << process.getProcessID() << ": " << (canHandle ? "Yes" : "No") << "\n";
    return canHandle;
}

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
            double resourceFitScore = (0.6 * node.getAvailableCpu() / std::max(1e-6, remainingCpu)) + // CPU contribution
                          (0.3 * node.getAvailableMemory() / std::max(1e-6, remainingMem)) + // Memory contribution
                          (0.1 * node.getBandwidth() / std::max(1e-6, remainingBw)); // Bandwidth contribution
            nodeQueue.emplace(resourceFitScore, node.getNodeID()); 
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
            allProcesses.push_back({process, {}, false, -1.0, -1.0});
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
            allProcesses.push_back({process, {}, false, -1.0, -1.0});
        }
    }
    std::cout << "Retry queue processing completed.\n";
}

// Scheduling method using greedy heuristic
void ContextAwareScheduler::schedule() {
    std::cout << "Context-Aware Scheduling started with greedy heuristic. Queue size: " << processPriorityQueue.size() << "\n";
    if (processPriorityQueue.empty()) return;

    // Track node availability
    std::unordered_map<int, double> nodeAvailableTime;
    for (const auto& node : fogNodes) {
        nodeAvailableTime[node.getNodeID()] = 0.0;
    }

    // Process each process directly from the priority queue in arrival order
    while (!processPriorityQueue.empty()) {
        auto process = getNextProcess();
        double arrival_time = static_cast<double>(process->getArrivalTime());
        auto resources = process->getRequiredResources();
        double burst_time = static_cast<double>(process->getBurstTime());
        std::vector<int> assignedNodes;
        double remaining_cpu = resources.cpu;
        double remaining_mem = resources.memory;
        double remaining_bw = process->getRequiredBandwidth();
        double process_start_time = arrival_time;
        double process_completion_time = arrival_time;

        // Structure to hold node assignment details
        struct NodeAssignment {
            int nodeId;
            double fraction;
            double start_time;
            double completion_time;
        };
        std::vector<NodeAssignment> assignments;

        // Assign process or its fractions
        while (remaining_cpu > 0.001 && remaining_mem > 0.001 && remaining_bw > 0.001) { // if resources are still available
            // Find the best node based on earliest finish time
            std::vector<std::pair<double, int>> node_finish_times; // {heuristic_rank, nodeId}
            for (auto& node : fogNodes) {
                int nodeId = node.getNodeID();
                if (node.getIsActive() && canNodeHandleProcess(node, *process)) {
                    double start_time = std::max(arrival_time, nodeAvailableTime[nodeId]);
                    double nodeScore = calculateNodeScore(node, process);
                    double processScore = process->getProcessScore();
                    double completion_time = start_time + burst_time; // Initial estimate
                    double heuristic_rank = completion_time - (nodeScore + processScore);
                    node_finish_times.emplace_back(heuristic_rank, nodeId); // Adjust by score
                }
            }

            if (node_finish_times.empty()) {
                std::cout << "No suitable node found for Process " << process->getProcessID() << ".\n";
                retryQueue.push(process);
                break;
            }

            // Sort by earliest finish time
            std::sort(node_finish_times.begin(), node_finish_times.end());

            int bestNodeId = node_finish_times[0].second; // Node with best heuristic score
            FogNode& bestNode = *nodeMap[bestNodeId];
            double start_time = std::max(arrival_time, nodeAvailableTime[bestNodeId]); // nodeAvailableTime of the best node

            // Determine maximum fraction that can be assigned
            double cpu_fraction = std::min(1.0, bestNode.getAvailableCpu() / std::max(1e-6, remaining_cpu));
            double mem_fraction = std::min(1.0, bestNode.getAvailableMemory() / std::max(1e-6, remaining_mem));
            double bw_fraction = std::min(1.0, (bestNode.getBandwidth() - nodeUsedBandwidth[bestNodeId]) / std::max(1e-6, remaining_bw));
            double fraction = std::min({cpu_fraction, mem_fraction, bw_fraction});

            if (fraction < 0.001) { // 0.0
                std::cout << "Insufficient resources on Node " << bestNodeId << " for Process " << process->getProcessID() << ".\n";
                retryQueue.push(process);
                break;
            }

            // Create partitioned process
            auto partitionedProcess = std::make_shared<Process>(*process);
            partitionedProcess->setRequiredResources(remaining_cpu * fraction, remaining_mem * fraction);
            partitionedProcess->setRequiredBandwidth(remaining_bw * fraction);

            if (bestNode.assignProcess(*partitionedProcess)) {
                double fraction_duration = burst_time * fraction;
                double completion_time = start_time + fraction_duration;
                nodeUsedBandwidth[bestNodeId] += remaining_bw * fraction;
                assignedNodes.push_back(bestNodeId);
                assignments.push_back({bestNodeId, fraction, start_time, completion_time});
                nodeAvailableTime[bestNodeId] = completion_time;
                updateNodeIndices(bestNodeId);

                // Update remaining resources
                remaining_cpu -= remaining_cpu * fraction;
                remaining_mem -= remaining_mem * fraction;
                remaining_bw -= remaining_bw * fraction;

                std::cout << "Assigned fraction " << fraction << " of Process " << process->getProcessID() 
                          << " to Node " << bestNodeId << " (Start: " << start_time << ", End: " << completion_time << ").\n";
            } else {
                std::cout << "Failed to assign fraction to Node " << bestNodeId << " for Process " << process->getProcessID() << ".\n";
                retryQueue.push(process);
                break;
            }
        }

        if (assignedNodes.empty()) {
            allProcesses.push_back({process, {}, false, -1.0, -1.0});
        } else {
            // Determine overall start and completion times
            double earliest_start = assignments[0].start_time;
            double latest_completion = assignments[0].completion_time;
            for (const auto& assignment : assignments) {
                earliest_start = std::min(earliest_start, assignment.start_time);
                latest_completion = std::max(latest_completion, assignment.completion_time);
            }
            processToNodeMap[process->getProcessID()] = assignedNodes;
            scheduledProcesses.push_back({process, assignedNodes});
            allProcesses.push_back({process, assignedNodes, true, earliest_start, latest_completion});
            std::cout << "Process " << process->getProcessID() << " fully assigned to " << assignedNodes.size() << " node(s).\n";
        }
    }

    // Handle retries
    processRetryQueue();
    std::cout << "Context-Aware Scheduling completed.\n";
}

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

void ContextAwareScheduler::printSchedulingMetrics() const {
    std::cout << "\n========================================\n";
    std::cout << "    Context-Aware Scheduling Metrics    \n";
    std::cout << "========================================\n";

    double total_waiting_time = 0.0;
    double total_turnaround_time = 0.0;
    double min_arrival = std::numeric_limits<double>::max();
    double max_completion = std::numeric_limits<double>::lowest();
    int scheduled_count = 0;
    int total_processes = allProcesses.size();
    
    std::unordered_map<int, double> node_utilization_time;
    double total_cpu_used = 0.0;
    double total_memory_used = 0.0;
    double total_bandwidth_used = 0.0;
    double total_cpu_capacity = 0.0;
    double total_memory_capacity = 0.0;
    double total_bandwidth_capacity = 0.0;

    for (const auto& node : fogNodes) {
        total_cpu_capacity += node.getCpuCapacity();
        total_memory_capacity += node.getMemory();
        total_bandwidth_capacity += node.getBandwidth();
        node_utilization_time[node.getNodeID()] = 0.0;
    }

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
            
            auto resources = process->getRequiredResources();
            double process_duration = static_cast<double>(process->getBurstTime());
            
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

    double total_execution_time = (scheduled_count > 0) ? (max_completion - min_arrival) : 0;
    double avg_waiting_time = (scheduled_count > 0) ? (total_waiting_time / scheduled_count) : 0;
    double avg_turnaround_time = (scheduled_count > 0) ? (total_turnaround_time / scheduled_count) : 0;
    double throughput = (total_execution_time > 0) ? (static_cast<double>(scheduled_count) / total_execution_time) : 0;
    double success_rate = (total_processes > 0) ? (static_cast<double>(scheduled_count) / total_processes * 100.0) : 0;
    
    double cpu_utilization = (total_cpu_capacity * total_execution_time > 0) ? 
                            (total_cpu_used / (total_cpu_capacity * total_execution_time) * 100.0) : 0;
    double memory_utilization = (total_memory_capacity * total_execution_time > 0) ? 
                               (total_memory_used / (total_memory_capacity * total_execution_time) * 100.0) : 0;
    double bandwidth_utilization = (total_bandwidth_capacity * total_execution_time > 0) ? 
                                  (total_bandwidth_used / (total_bandwidth_capacity * total_execution_time) * 100.0) : 0;

    int total_calculations = processScoreCount + nodeScoreCount + resourceCheckCount + 
                           retryAttemptCount + partitioningAttemptCount;

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