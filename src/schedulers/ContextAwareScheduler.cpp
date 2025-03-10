#include "ContextAwareScheduler.h"
#include <iostream>
#include <cmath>
#include <limits>
#include <algorithm>

// Constructor
ContextAwareScheduler::ContextAwareScheduler(const std::vector<FogNode>& nodes) 
    : fogNodes(nodes), sortedNodes(nodes) {}

// Add process to the queue
void ContextAwareScheduler::addProcess(std::shared_ptr<Process> process) {
    BaseScheduler::addProcess(process);
    std::cout << "Process " << process->getProcessID() << " added to queue.\n";
}

// Calculate location-based scoring
double ContextAwareScheduler::calculateLocationScore(const std::shared_ptr<Process>& process, const FogNode& node) {
    if (process->getRequestLocation() == node.getLocation()) {
        return 1.0;  // Perfect location match
    } else {
        return std::exp(-0.5);  // Significant but not total penalty
    }
}

// Calculate load balance scoring with safe bounds
double ContextAwareScheduler::calculateLoadBalanceScore(const FogNode& node) {
    return 1.0 / (1.0 + std::exp(std::min(node.getCurrentLoad() * 5, 10.0)));
}

// Select next process based on comprehensive scoring
std::shared_ptr<Process> ContextAwareScheduler::getNextProcess() {
    if (processQueue.empty()) return nullptr;

    std::shared_ptr<Process> bestProcess = nullptr;
    double bestScore = std::numeric_limits<double>::lowest();

    for (const auto& process : processQueue) {
        double score = 
            (1.0 / (1.0 + process->getPriority())) + // Priority consideration
            (0.5 * (1.0 - process->getMobility())) + // Mobility factor
            (0.2 * process->getNps()) + // Network Performance Score
            (0.3 * (1.0 - process->getRelinquishProbability())) + // Stability factor
            (0.4 * process->getLatencySensitivity()); // Latency sensitivity

        if (score > bestScore) {
            bestScore = score;
            bestProcess = process;
        }
    }

    return bestProcess;
}

// Helper function to check if a node can handle a process
bool ContextAwareScheduler::canNodeHandleProcess(const FogNode& node, const Process& process) {
    return node.getDelay() <= process.getMaxDelay() &&
           node.getPacketLoss() <= process.getMaxPacketLoss();
}

// Main scheduling method with optimized sorting and retry queue
void ContextAwareScheduler::schedule() {
    std::cout << "Scheduling started. Queue size: " << processQueue.size() << "\n";
    
    if (processQueue.empty()) {
        std::cout << "No processes to schedule.\n";
        return;
    }

    // Sort nodes by load once for reuse
    sortedNodes = fogNodes;
    std::sort(sortedNodes.begin(), sortedNodes.end(), 
        [](const FogNode& a, const FogNode& b) {
            return a.getCurrentLoad() < b.getCurrentLoad();
        });

    while (!processQueue.empty()) {
        std::shared_ptr<Process> process = getNextProcess();
        if (!process) break;  // No process selected (shouldn't happen due to empty check)

        auto it = std::find(processQueue.begin(), processQueue.end(), process);
        std::iter_swap(it, processQueue.end() - 1);
        processQueue.pop_back();

        int assignedNode = assignToFogNode(process);
        if (assignedNode != -1) {
            for (auto& node : fogNodes) {
                if (node.getNodeID() == assignedNode) {
                    if (node.assignProcess(*process)) {
                        std::cout << "Process " << process->getProcessID() 
                                  << " assigned to Fog Node " << assignedNode 
                                  << " (Location: " << node.getLocation() << ").\n";
                        processToNodeMap[process->getProcessID()] = {assignedNode};
                    } else {
                        std::cout << "Fog Node " << assignedNode 
                                  << " failed to assign Process " << process->getProcessID() 
                                  << " due to load constraints.\n";
                        if (!partitionProcess(process)) {
                            std::cout << "Partitioning failed for Process " 
                                      << process->getProcessID() << ". Adding to retry queue.\n";
                            retryQueue.push_back(process);
                        }
                    }
                    break;
                }
            }
        } else {
            std::cout << "No suitable Fog Node found for Process " 
                      << process->getProcessID() << ". Attempting advanced partitioning...\n";
            if (!partitionProcess(process)) {
                std::cout << "Advanced partitioning failed for Process " 
                          << process->getProcessID() << ". Adding to retry queue.\n";
                retryQueue.push_back(process);
            }
        }
    }

    // Retry failed processes
    if (!retryQueue.empty()) {
        std::cout << "Retrying " << retryQueue.size() << " failed processes...\n";
        std::vector<std::shared_ptr<Process>> tempQueue = std::move(retryQueue);
        retryQueue.clear();
        for (auto& process : tempQueue) {
            if (!partitionProcess(process)) {
                std::cout << "Retry failed for Process " << process->getProcessID() 
                          << ". Escalation to higher-tier scheduler recommended.\n";
            } else {
                std::cout << "Retry succeeded for Process " << process->getProcessID() << ".\n";
            }
        }
    }
}

// Assign process to the most suitable Fog Node
int ContextAwareScheduler::assignToFogNode(std::shared_ptr<Process> process) {
    int bestNode = -1;
    double bestScore = std::numeric_limits<double>::lowest();

    for (auto& node : sortedNodes) {
        if (!node.getIsActive()) {
            std::cout << "Fog Node " << node.getNodeID() << " is inactive.\n";
            continue;
        }

        auto resources = process->getRequiredResources();

        // Allow partial assignment if at least 50% of resources are available
        bool resourcesFit = 
            (node.getCpuCapacity() >= resources.cpu * 0.5) && // Partial assignment threshold
            node.getMemory() >= resources.memory &&
            node.getBandwidth() >= process->getRequiredBandwidth();

        if (resourcesFit && canNodeHandleProcess(node, *process)) {
            double newLoad = node.getCurrentLoad() + (resources.cpu / node.getCpuCapacity());
            if (newLoad <= 1.0) {
                // Calculate scoring components for the Comprehensive Scoring Mechanism
                double locationScore = calculateLocationScore(process, node);
                double loadBalanceScore = calculateLoadBalanceScore(node);

                // Multifactor scoring
                double score = 
                    (1.0 / (1.0 + node.getDelay())) + // Delay preference
                    (node.getBandwidth() / process->getRequiredBandwidth()) + // Bandwidth utilization
                    (1.0 - (node.getPacketLoss() / process->getMaxPacketLoss())) + // Packet loss
                    locationScore + // Location matching
                    loadBalanceScore; // Load distribution

                std::cout << "Fog Node " << node.getNodeID() 
                          << " score: " << score 
                          << " (newLoad: " << newLoad 
                          << ", Location: " << node.getLocation() << ")\n";

                if (score > bestScore) {
                    bestScore = score;
                    bestNode = node.getNodeID();
                }
            } else {
                std::cout << "Fog Node " << node.getNodeID() 
                          << " rejected due to high load: " << newLoad << "\n";
            }
        } else {
            std::cout << "Fog Node " << node.getNodeID() 
                      << " rejected due to resource constraints.\n";
        }
    }

    return bestNode;
}

// Partition process across multiple nodes with recursion limit
bool ContextAwareScheduler::partitionProcess(std::shared_ptr<Process> process) {
    static int recursionDepth = 0;
    if (recursionDepth++ > 10) {  // Prevent infinite recursion
        recursionDepth = 0;
        std::cout << "Recursion depth exceeded for Process " << process->getProcessID() << ". Partitioning aborted.\n";
        return false;
    }

    auto resources = process->getRequiredResources();
    double remainingCpu = resources.cpu;
    double remainingMem = resources.memory;
    double remainingBw = process->getRequiredBandwidth();
    std::vector<int> assignedNodes;

    for (auto& node : sortedNodes) {
        if (!node.getIsActive() || node.getCurrentLoad() >= 1.0) continue;

        double cpuAvailable = node.getCpuCapacity() * (1.0 - node.getCurrentLoad());
        double memAvailable = node.getMemory() * (1.0 - node.getCurrentLoad());
        double bwAvailable = node.getBandwidth();

        if (canNodeHandleProcess(node, *process)) {
            double cpuToAssign = std::min(remainingCpu, cpuAvailable);
            double memToAssign = std::min(remainingMem, memAvailable);
            double bwToAssign = std::min(remainingBw, bwAvailable);

            if (cpuToAssign > 0 && memToAssign > 0 && bwToAssign > 0) {
                auto partitionedProcess = std::make_shared<Process>(*process);
                partitionedProcess->setRequiredResources(cpuToAssign, memToAssign);
                
                if (node.assignProcess(*partitionedProcess)) {
                    processToNodeMap[process->getProcessID()].push_back(node.getNodeID()); // Add node ID to the vector
                    assignedNodes.push_back(node.getNodeID());

                    remainingCpu -= cpuToAssign;
                    remainingMem -= memToAssign;
                    remainingBw -= bwToAssign;

                    std::cout << "Partitioned Process " << process->getProcessID() 
                              << " partially assigned to Fog Node " << node.getNodeID() 
                              << " (CPU: " << cpuToAssign 
                              << ", Memory: " << memToAssign << ")\n";
                }

                if (remainingCpu <= 0 && remainingMem <= 0 && remainingBw <= 0) {
                    std::cout << "Process " << process->getProcessID() 
                              << " fully partitioned across nodes: ";
                    for (int id : assignedNodes) std::cout << id << " ";
                    std::cout << "\n";
                    recursionDepth = 0;
                    return true;
                }
            }
        }
    }

    if (remainingCpu > 0 || remainingMem > 0 || remainingBw > 0) {
        std::cout << "Remaining resources for Process " << process->getProcessID() 
                  << " (CPU: " << remainingCpu << ", Mem: " << remainingMem 
                  << ", BW: " << remainingBw << "). Retrying partitioning...\n";
        bool result = partitionProcess(process);
        recursionDepth--;
        return result;
    }

    recursionDepth = 0;
    return false;
}

// Print the current scheduling state
void ContextAwareScheduler::printSchedulingState() const {
    std::cout << "Current Scheduling State:\n";
    for (const auto& entry : processToNodeMap) {
        if (entry.second.empty()) {
            std::cout << "Process " << entry.first << " -> No Fog Node assigned\n";
        } else {
            std::cout << "Process " << entry.first << " -> Fog Node(s): ";
            for (int nodeId : entry.second) {
                std::cout << nodeId << " ";
            }
            std::cout << "\n";
        }
    }
}