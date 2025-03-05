#include "PreemptiveScheduler.h"
#include <iostream>
#include <limits>
#include <cmath>
#include <algorithm>

PreemptiveScheduler::PreemptiveScheduler(const std::vector<FogNode>& nodes) : fogNodes(nodes) {}

void PreemptiveScheduler::schedule() {
    if (processQueue.empty()) {
        std::cout << "No processes to schedule.\n";
        return;
    }

    while (!processQueue.empty()) {
        std::shared_ptr<Process> process = getNextProcess();
        if (!process) break;

        int assignedNode = assignToFogNode(process);
        if (assignedNode != -1) {
            std::cout << "Process " << process->getProcessID() << " assigned to Fog Node " << assignedNode << ".\n";
            processToNodeMap[process->getProcessID()] = assignedNode;
        } else {
            std::cout << "No suitable Fog Node found for Process " << process->getProcessID() << ".\n";
        }

        processQueue.erase(std::remove_if(processQueue.begin(), processQueue.end(),
                                          [&](const std::shared_ptr<Process>& p) {
                                              return p->getProcessID() == process->getProcessID();
                                          }),
                           processQueue.end());
    }
}

std::shared_ptr<Process> PreemptiveScheduler::getNextProcess() {
    if (processQueue.empty()) return nullptr;

    std::shared_ptr<Process> bestProcess = nullptr;
    double bestScore = std::numeric_limits<double>::lowest();

    for (const auto& process : processQueue) {
        double score = (1.0 / (1.0 + process->getPriority()))
                     + (0.5 * (1.0 - process->getExecutionTime() / process->getDeadline()))
                     + (0.4 * process->getLatencySensitivity());

        if (score > bestScore) {
            bestScore = score;
            bestProcess = process;
        }
    }

    return bestProcess;
}

int PreemptiveScheduler::assignToFogNode(std::shared_ptr<Process> process) {
    int bestNode = -1;
    double bestScore = std::numeric_limits<double>::lowest();
    std::shared_ptr<Process> processToPreempt = nullptr;

    for (auto& node : fogNodes) {
        if (!node.getIsActive()) continue;

        std::shared_ptr<Process> runningProcess = node.getRunningProcess();
        auto resources = process->getRequiredResources();
        double processScore = (1.0 / (1.0 + node.getDelay()))
                            + (node.getBandwidth() / process->getRequiredBandwidth())
                            - (std::abs(node.getProcessingPower() - resources.cpu));  // Use .cpu

        if (runningProcess) {
            auto runningResources = runningProcess->getRequiredResources();
            double runningProcessScore = (1.0 / (1.0 + node.getDelay()))
                                       + (node.getBandwidth() / runningProcess->getRequiredBandwidth())
                                       - (std::abs(node.getProcessingPower() - runningResources.cpu));  // Use .cpu

            if (process->getPriority() < runningProcess->getPriority() && processScore > runningProcessScore) {
                processToPreempt = runningProcess;
                bestNode = node.getNodeID();
                bestScore = processScore;
            }
        } else if (processScore > bestScore) {
            bestScore = processScore;
            bestNode = node.getNodeID();
        }
    }

    if (processToPreempt) {
        std::cout << "Preempting Process " << processToPreempt->getProcessID()
                  << " for Process " << process->getProcessID() << " on Fog Node " << bestNode << ".\n";
    }

    return bestNode;
}

void PreemptiveScheduler::printSchedulingState() const {
    std::cout << "Current Scheduling State:\n";
    for (const auto& entry : processToNodeMap) {
        std::cout << "Process " << entry.first << " -> Fog Node " << entry.second << "\n";
    }
}