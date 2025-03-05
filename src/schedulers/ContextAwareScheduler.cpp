#include "ContextAwareScheduler.h"
#include <iostream>
#include <limits>
#include <cmath>
#include <algorithm>
#include <memory>

ContextAwareScheduler::ContextAwareScheduler(const std::vector<FogNode>& nodes) : fogNodes(nodes) {}

void ContextAwareScheduler::addProcess(std::shared_ptr<Process> process) {
    processQueue.push_back(process);
    std::cout << "Process " << process->getProcessID() << " added to queue.\n";
}

void ContextAwareScheduler::schedule() {
    std::cout << "Scheduling started. Queue size: " << processQueue.size() << "\n";
    if (processQueue.empty()) {
        std::cout << "No processes to schedule.\n";
        return;
    }

    while (!processQueue.empty()) {
        std::shared_ptr<Process> process = getNextProcess();
        if (!process) break;

        int assignedNode = assignToFogNode(process);
        if (assignedNode != -1) {
            for (auto& node : fogNodes) {
                if (node.getNodeID() == assignedNode) {
                    if (node.assignProcess(*process)) {
                        std::cout << "Process " << process->getProcessID() << " assigned to Fog Node " << assignedNode << ".\n";
                        processToNodeMap[process->getProcessID()] = assignedNode;
                    } else {
                        std::cout << "Fog Node " << assignedNode << " failed to assign Process " << process->getProcessID() << " due to load.\n";
                    }
                    break;
                }
            }
        } else {
            std::cout << "No suitable Fog Node found for Process " << process->getProcessID() << ". Attempting partitioning...\n";
            if (!partitionProcess(process)) {
                std::cout << "Partitioning failed for Process " << process->getProcessID() << ".\n";
            }
        }

        processQueue.erase(std::remove_if(processQueue.begin(), processQueue.end(),
                                          [&](const std::shared_ptr<Process>& p) {
                                              return p->getProcessID() == process->getProcessID();
                                          }),
                           processQueue.end());
    }
}

std::shared_ptr<Process> ContextAwareScheduler::getNextProcess() {
    if (processQueue.empty()) return nullptr;

    std::shared_ptr<Process> bestProcess = nullptr;
    double bestScore = std::numeric_limits<double>::lowest();

    for (const auto& process : processQueue) {
        double score = (1.0 / (1.0 + process->getPriority()))
                     + (0.5 * (1.0 - process->getMobility()))
                     + (0.2 * process->getNps())
                     + (0.3 * (1.0 - process->getRelinquishProbability()))
                     + (0.4 * process->getLatencySensitivity());

        if (score > bestScore) {
            bestScore = score;
            bestProcess = process;
        }
    }

    return bestProcess;
}

int ContextAwareScheduler::assignToFogNode(std::shared_ptr<Process> process) {
    int bestNode = -1;
    double bestScore = std::numeric_limits<double>::lowest();

    for (auto& node : fogNodes) {
        if (!node.getIsActive()) {
            std::cout << "Fog Node " << node.getNodeID() << " is inactive.\n";
            continue;
        }

        auto resources = process->getRequiredResources();
        double cpuAvailable = node.getCpuCapacity() * (1.0 - node.getCurrentLoad());
        double memAvailable = node.getMemory() * (1.0 - node.getCurrentLoad());
        double bwAvailable = node.getBandwidth();

        bool cpuFit = node.getCpuCapacity() >= resources.cpu;
        bool memFit = node.getMemory() >= resources.memory;
        bool bwFit = bwAvailable >= process->getRequiredBandwidth();
        bool delayFit = node.getDelay() <= process->getMaxDelay();
        bool plFit = node.getPacketLoss() <= process->getMaxPacketLoss();

        if (cpuFit && memFit && bwFit && delayFit && plFit) {
            double newLoad = node.getCurrentLoad() + (resources.cpu / node.getCpuCapacity());
            if (newLoad <= 1.0) {  // Pre-check load to avoid calling assignProcess unnecessarily
                double score = (1.0 / (1.0 + node.getDelay()))
                             + (bwAvailable / process->getRequiredBandwidth())
                             + (1.0 - (node.getPacketLoss() / process->getMaxPacketLoss()))
                             + (1.0 - (resources.cpu / node.getCpuCapacity()))
                             + (1.0 - (resources.memory / node.getMemory()));

                std::cout << "Fog Node " << node.getNodeID() << " score: " << score << " (newLoad: " << newLoad << ")\n";
                if (score > bestScore) {
                    bestScore = score;
                    bestNode = node.getNodeID();
                }
            } else {
                std::cout << "Fog Node " << node.getNodeID() << " rejected due to load: " << newLoad << "\n";
            }
        } else {
            std::cout << "Fog Node " << node.getNodeID() << " rejected: "
                      << (cpuFit ? "" : "CPU ") << (memFit ? "" : "Memory ")
                      << (bwFit ? "" : "Bandwidth ") << (delayFit ? "" : "Delay ")
                      << (plFit ? "" : "Packet Loss ") << "\n";
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

    for (auto& node : fogNodes) {
        if (!node.getIsActive() || node.getCurrentLoad() >= 1.0) continue;

        double cpuAvailable = node.getCpuCapacity() * (1.0 - node.getCurrentLoad());
        double memAvailable = node.getMemory() * (1.0 - node.getCurrentLoad());
        double bwAvailable = node.getBandwidth();

        if (node.getDelay() <= process->getMaxDelay() && node.getPacketLoss() <= process->getMaxPacketLoss()) {
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

                    std::cout << "Partitioned Process " << process->getProcessID() << " assigned "
                              << cpuToAssign << " CPU, " << memToAssign << " Memory to Fog Node " << node.getNodeID() << "\n";
                }

                if (remainingCpu <= 0 && remainingMem <= 0 && remainingBw <= 0) {
                    std::cout << "Process " << process->getProcessID() << " fully partitioned across nodes: ";
                    for (int id : assignedNodes) std::cout << id << " ";
                    std::cout << "\n";
                    return true;
                }
            }
        }
    }

    return false;
}

void ContextAwareScheduler::printSchedulingState() const {
    std::cout << "Current Scheduling State:\n";
    for (const auto& entry : processToNodeMap) {
        std::cout << "Process " << entry.first << " -> Fog Node " << entry.second << "\n";
    }
}