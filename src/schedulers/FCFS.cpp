#include "FCFS.h"
#include <iostream>
#include <algorithm>
#include <limits>

// Initialize static counters
int FCFSScheduler::resourceCheckCount = 0;
int FCFSScheduler::assignmentAttemptCount = 0;

// Constructor
FCFSScheduler::FCFSScheduler(const std::vector<FogNode>& nodes)
    : fogNodes(nodes) {
    // Initialize node mappings
    for (const auto& node : fogNodes) {
        int nodeId = node.getNodeID();
        nodeMap[nodeId] = const_cast<FogNode*>(&node);
        nodeUsedBandwidth[nodeId] = 0.0; // Initialize used bandwidth
    }
    std::cout << "FCFSScheduler initialized with " << fogNodes.size() << " nodes.\n";
}

// Add process to the queue
void FCFSScheduler::addProcess(std::shared_ptr<Process> process) {
    BaseScheduler::addProcess(process);
    processQueue.push(process);
    std::cout << "Process " << process->getProcessID() << " added to FCFS queue.\n";
}

// Get the next process (FCFS: earliest arrival time)
std::shared_ptr<Process> FCFSScheduler::getNextProcess() {
    if (processQueue.empty()) {
        return nullptr;
    }
    auto process = processQueue.front();
    processQueue.pop();
    return process;
}

// Check if a node can handle a process
bool FCFSScheduler::canNodeHandleProcess(const FogNode& node, const Process& process) const {
    bool canHandle = node.getDelay() <= process.getMaxDelay() &&
                     node.getPacketLoss() <= process.getMaxPacketLoss();
    std::cout << "Node " << node.getNodeID() << " can handle Process " << process.getProcessID()
              << ": " << (canHandle ? "Yes" : "No") << "\n";
    return canHandle;
}

// Check if resources can fit a process
bool FCFSScheduler::canResourcesFit(const FogNode& node, const Resource& resources, const Process& process) const {
    resourceCheckCount++;
    int nodeId = node.getNodeID();
    double remainingBandwidth = node.getBandwidth() - nodeUsedBandwidth.at(nodeId);
    bool fits = node.getAvailableCpu() >= resources.cpu &&
                node.getAvailableMemory() >= resources.memory &&
                remainingBandwidth >= process.getRequiredBandwidth();
    std::cout << "Resources fit check for Node " << nodeId << " and Process " << process.getProcessID()
              << ": " << (fits ? "Yes" : "No") << "\n";
    return fits;
}

// Update node indices (simplified for FCFS)
void FCFSScheduler::updateNodeIndices(int nodeId) {
    if (nodeMap.find(nodeId) == nodeMap.end()) return;
    std::cout << "Node " << nodeId << " indices updated.\n";
}

// Assign process to the first suitable fog node
int FCFSScheduler::assignToFogNode(std::shared_ptr<Process> process) {
    assignmentAttemptCount++;
    auto resources = process->getRequiredResources();

    for (auto& node : fogNodes) {
        int nodeId = node.getNodeID();
        if (!node.getIsActive()) continue;

        if (canResourcesFit(node, resources, *process) && canNodeHandleProcess(node, *process)) {
            double cpuLoad = node.getCurrentLoad() + (resources.cpu / std::max(1e-6, node.getCpuCapacity()));
            double bandwidthLoad = (nodeUsedBandwidth[nodeId] + process->getRequiredBandwidth()) / node.getBandwidth();
            double overallLoad = std::max(cpuLoad, bandwidthLoad);
            if (overallLoad <= 1.0) {
                if (node.assignProcess(*process)) {
                    nodeUsedBandwidth[nodeId] += process->getRequiredBandwidth();
                    std::cout << "Assigned Process " << process->getProcessID()
                              << " to Node " << nodeId << ".\n";
                    updateNodeIndices(nodeId);
                    return nodeId;
                }
            }
        }
    }

    std::cout << "No suitable node found for Process " << process->getProcessID() << ".\n";
    return -1;
}

// Main scheduling method
void FCFSScheduler::schedule() {
    std::cout << "FCFS Scheduling started. Queue size: " << processQueue.size() << "\n";
    if (processQueue.empty()) return;

    // Map to track the earliest available time for each node
    std::unordered_map<int, double> nodeAvailableTime;
    for (const auto& node : fogNodes) {
        nodeAvailableTime[node.getNodeID()] = 0.0; // Initially available at time 0
    }

    while (!processQueue.empty()) {
        auto process = getNextProcess();
        double arrival_time = static_cast<double>(process->getArrivalTime());
        int assignedNode = assignToFogNode(process);

        if (assignedNode != -1) {
            // Start time is the maximum of arrival time and node's available time
            double start_time = std::max(arrival_time, nodeAvailableTime[assignedNode]);
            double completion_time = start_time + static_cast<double>(process->getBurstTime());
            nodeAvailableTime[assignedNode] = completion_time; // Update node's available time
            std::cout << "Process " << process->getProcessID() << " assigned to Node " << assignedNode << ".\n";
            processToNodeMap[process->getProcessID()] = {assignedNode};
            scheduledProcesses.push_back({process, {assignedNode}});
            allProcesses.push_back({process, {assignedNode}, true, start_time, completion_time});
        } else {
            std::cout << "Failed to assign Process " << process->getProcessID() << ".\n";
            allProcesses.push_back({process, {}, false, -1.0, -1.0});
        }
    }

    std::cout << "FCFS Scheduling completed.\n";
}

// Print summary of scheduled processes
void FCFSScheduler::printSchedulingSummary() const {
    std::cout << "\n=== FCFS Scheduled Processes Summary ===\n";
    std::cout << "Process_ID,Arrival_Time,Burst_Time,Start_Time,Completion_Time,Waiting_Time,Turnaround_Time,Assigned_Node,Status\n";
    
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
                      << turnaround_time << ","
                      << nodeIds[0] << ",Scheduled\n";
        } else {
            std::cout << "-1,-1,-1,-1,-1,Failed\n";
        }
    }
    std::cout << "==========================================\n";
}

// Print scheduling metrics
void FCFSScheduler::printSchedulingMetrics() const {
    std::cout << "\n========================================\n";
    std::cout << "         FCFS Scheduling Metrics        \n";
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
            int nodeId = nodeIds[0];
            
            node_utilization_time[nodeId] += process_duration;
            total_cpu_used += resources.cpu * process_duration;
            total_memory_used += resources.memory * process_duration;
            total_bandwidth_used += process->getRequiredBandwidth() * process_duration;
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
    int total_calculations = resourceCheckCount + assignmentAttemptCount;

    // Print summary metrics in CSV format for easy plotting
    std::cout << "\n=== DETAILED SUMMARY METRICS (CSV FORMAT) ===\n";
    std::cout << "Metric,Value\n";
    std::cout << "Algorithm,FCFS\n";
    std::cout << "Total Processes," << total_processes << "\n";
    std::cout << "Scheduled Processes," << scheduled_count << "\n";
    std::cout << "Failed Processes," << (total_processes - scheduled_count) << "\n";
    std::cout << "Success Rate," << success_rate << "%\n";
    std::cout << "Total Execution Time," << total_execution_time << " units\n";
    std::cout << "Average Waiting Time," << avg_waiting_time << " units\n";
    std::cout << "Average Turnaround Time," << avg_turnaround_time << " units\n";
    std::cout << "Throughput," << throughput << " processes/unit\n";
    std::cout << "CPU Utilization," << cpu_utilization << "%\n";
    std::cout << "Memory Utilization Percent," << memory_utilization << "%\n";
    std::cout << "Bandwidth Utilization Percent," << bandwidth_utilization << "%\n";
    std::cout << "Scheduling Overhead," << total_calculations << " calculations\n";
    std::cout << "Resource Checks," << resourceCheckCount << "\n";
    std::cout << "Assignment Attempts," << assignmentAttemptCount << "\n";

    std::cout << "\n=== NODE UTILIZATION ===\n";
    std::cout << "Node_ID,Utilization_Time,Utilization_Percent\n";
    for (const auto& [nodeId, util_time] : node_utilization_time) {
        double util_percent = (total_execution_time > 0) ? (util_time / total_execution_time * 100.0) : 0;
        std::cout << nodeId << "," << util_time << "," << util_percent << "\n";
    }

    std::cout << "\nEnd of FCFS Scheduling Analysis\n";
    std::cout << "========================================\n";
}