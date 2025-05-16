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
    for (size_t i = 0; i < allProcesses.size(); ++i) {
        const auto& [process, nodeIds, isScheduled, start_time, completion_time] = allProcesses[i];
        std::cout << "Process " << process->getProcessID() << " ";
        if (i == 0) {
            std::cout << "is scheduled first and ";
        }
        if (!isScheduled) {
            std::cout << "not scheduled.\n";
        } else {
            std::cout << "assigned to Node " << nodeIds[0] << ".\n";
        }
    }
    std::cout << "=====================================\n";
}

// Print scheduling metrics
void FCFSScheduler::printSchedulingMetrics() const {
    std::cout << "\n========================================\n";
    std::cout << "       FCFS Scheduling Summary Report    \n";
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
            std::cout << "  Assigned to Node(s): " << nodeIds[0] << "\n";
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
    // std::cout << "Memory Utilization: " << (memory_utilization * 100) << "%\n";
    std::cout << "Fairness (Variance of Waiting Times): " << variance << "\n";
    std::cout << "----------------------------------------\n";

    // Calculation counts
    int total_calculations = resourceCheckCount + assignmentAttemptCount;
    std::cout << "\nCalculation Counts:\n";
    std::cout << "----------------------------------------\n";
    std::cout << "Resource Checks: " << resourceCheckCount << "\n";
    std::cout << "Assignment Attempts: " << assignmentAttemptCount << "\n";
    std::cout << "Scheduling Overhead (Total Calculations): " << total_calculations << "\n";
    std::cout << "----------------------------------------\n";

    std::cout << "\nEnd of FCFS Scheduling Summary\n";
    std::cout << "========================================\n";
}