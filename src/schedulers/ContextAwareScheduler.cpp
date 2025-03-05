#include "ContextAwareScheduler.h"
#include <iostream>
#include <cmath>

// Constructor
ContextAwareScheduler::ContextAwareScheduler(const std::vector<FogNode>& nodes) 
    : fogNodes(nodes) {}

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
        // Exponential decay for location mismatch
        return std::exp(-0.5);  // Significant but not total penalty
    }
}

// Calculate load balance scoring
double ContextAwareScheduler::calculateLoadBalanceScore(const FogNode& node) {
    // Prefer nodes with lower current load
    return 1.0 / (1.0 + std::exp(node.getCurrentLoad() * 5));
}

// Select next process based on comprehensive scoring
std::shared_ptr<Process> ContextAwareScheduler::getNextProcess() {
    if (processQueue.empty()) return nullptr;

    std::shared_ptr<Process> bestProcess = nullptr;
    double bestScore = std::numeric_limits<double>::lowest();

    for (const auto& process : processQueue) {
        // Comprehensive scoring mechanism
        double score = 
            (1.0 / (1.0 + process->getPriority())) +  // Priority consideration
            (0.5 * (1.0 - process->getMobility())) +  // Mobility factor
            (0.2 * process->getNps()) +  // Network Performance Score
            (0.3 * (1.0 - process->getRelinquishProbability())) +  // Stability factor
            (0.4 * process->getLatencySensitivity());  // Latency sensitivity

        if (score > bestScore) {
            bestScore = score;
            bestProcess = process;
        }
    }

    return bestProcess;
}

// Main scheduling method
void ContextAwareScheduler::schedule() {
    std::cout << "Scheduling started. Queue size: " << processQueue.size() << "\n";
    
    if (processQueue.empty()) {
        std::cout << "No processes to schedule.\n";
        return;
    }

    // Sort processes by priority and resource requirements
    std::sort(processQueue.begin(), processQueue.end(), 
        [](const std::shared_ptr<Process>& a, const std::shared_ptr<Process>& b) {
            // Primary sort by priority (lower number means higher priority)
            if (a->getPriority() != b->getPriority()) {
                return a->getPriority() < b->getPriority();
            }
            // Secondary sort by resource intensity
            auto resourcesA = a->getRequiredResources();
            auto resourcesB = b->getRequiredResources();
            return (resourcesA.cpu + resourcesA.memory) > (resourcesB.cpu + resourcesB.memory);
        });

    while (!processQueue.empty()) {
        std::shared_ptr<Process> process = processQueue.front();
        processQueue.erase(processQueue.begin());

        int assignedNode = assignToFogNode(process);
        if (assignedNode != -1) {
            for (auto& node : fogNodes) {
                if (node.getNodeID() == assignedNode) {
                    if (node.assignProcess(*process)) {
                        std::cout << "Process " << process->getProcessID() 
                                  << " assigned to Fog Node " << assignedNode 
                                  << " (Location: " << node.getLocation() << ").\n";
                        processToNodeMap[process->getProcessID()] = assignedNode;
                    } else {
                        std::cout << "Fog Node " << assignedNode 
                                  << " failed to assign Process " << process->getProcessID() 
                                  << " due to load constraints.\n";
                        // Attempt partitioning
                        if (!partitionProcess(process)) {
                            std::cout << "Rescheduling of Process " 
                                      << process->getProcessID() << " failed.\n";
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
                          << process->getProcessID() << ". Process dropped.\n";
            }
        }
    }
}

// Assign process to the most suitable Fog Node
int ContextAwareScheduler::assignToFogNode(std::shared_ptr<Process> process) {
    int bestNode = -1;
    double bestScore = std::numeric_limits<double>::lowest();

    for (auto& node : fogNodes) {
        if (!node.getIsActive()) {
            std::cout << "Fog Node " << node.getNodeID() << " is inactive.\n";
            continue;
        }

        auto resources = process->getRequiredResources();
        
        // Enhanced constraint checking
        bool resourcesFit = 
            node.getCpuCapacity() >= resources.cpu &&
            node.getMemory() >= resources.memory &&
            node.getBandwidth() >= process->getRequiredBandwidth() &&
            node.getDelay() <= process->getMaxDelay() &&
            node.getPacketLoss() <= process->getMaxPacketLoss();

        if (resourcesFit) {
            double newLoad = node.getCurrentLoad() + (resources.cpu / node.getCpuCapacity());
            
            if (newLoad <= 1.0) {
                // Comprehensive scoring mechanism
                double locationScore = calculateLocationScore(process, node);
                double loadBalanceScore = calculateLoadBalanceScore(node);
                
                // Multifactor scoring
                double score = 
                    (1.0 / (1.0 + node.getDelay())) +  // Delay preference
                    (node.getBandwidth() / process->getRequiredBandwidth()) +  // Bandwidth utilization
                    (1.0 - (node.getPacketLoss() / process->getMaxPacketLoss())) +  // Packet loss
                    locationScore +  // Location matching
                    loadBalanceScore;  // Load distribution

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

// Partition process across multiple nodes if initial assignment fails
bool ContextAwareScheduler::partitionProcess(std::shared_ptr<Process> process) {
    auto resources = process->getRequiredResources();
    double remainingCpu = resources.cpu;
    double remainingMem = resources.memory;
    double remainingBw = process->getRequiredBandwidth();
    std::vector<int> assignedNodes;

    // Sort nodes by their current load (lowest first)
    std::vector<FogNode> sortedNodes = fogNodes;
    std::sort(sortedNodes.begin(), sortedNodes.end(), 
        [](const FogNode& a, const FogNode& b) {
            return a.getCurrentLoad() < b.getCurrentLoad();
        });

    for (auto& node : sortedNodes) {
        if (!node.getIsActive() || node.getCurrentLoad() >= 1.0) continue;

        double cpuAvailable = node.getCpuCapacity() * (1.0 - node.getCurrentLoad());
        double memAvailable = node.getMemory() * (1.0 - node.getCurrentLoad());
        double bwAvailable = node.getBandwidth();

        // Location-aware partitioning
        if (node.getDelay() <= process->getMaxDelay() && 
            node.getPacketLoss() <= process->getMaxPacketLoss()) {
            
            double cpuToAssign = std::min(remainingCpu, cpuAvailable);
            double memToAssign = std::min(remainingMem, memAvailable);
            double bwToAssign = std::min(remainingBw, bwAvailable);

            if (cpuToAssign > 0 && memToAssign > 0 && bwToAssign > 0) {
                auto partitionedProcess = std::make_shared<Process>(*process);
                partitionedProcess->setRequiredResources(cpuToAssign, memToAssign);
                
                if (node.assignProcess(*partitionedProcess)) {
                    processToNodeMap[process->getProcessID()] = node.getNodeID();
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
                    return true;
                }
            }
        }
    }

    return false;
}

// Print the current scheduling state
void ContextAwareScheduler::printSchedulingState() const {
    std::cout << "Current Scheduling State:\n";
    for (const auto& entry : processToNodeMap) {
        std::cout << "Process " << entry.first << " -> Fog Node " << entry.second << "\n";
    }
}