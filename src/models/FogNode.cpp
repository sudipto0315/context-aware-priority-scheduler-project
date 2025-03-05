#include "FogNode.h"
#include <iostream>

FogNode::FogNode(int id, double cpu, double mem, double bw, double load, double dly, double pl, std::string loc, bool active)
    : nodeID(id), processingPower(cpu), memory(mem), bandwidth(bw), currentLoad(load),
      delay(dly), packetLoss(pl), location(loc), isActive(active), runningProcess(nullptr) {}

      bool FogNode::assignProcess(const Process& process) {
        auto resources = process.getRequiredResources();
        double newLoad = currentLoad + (resources.cpu / processingPower);
        std::cout << "Node " << nodeID << " newLoad: " << newLoad << "\n";
        if (newLoad <= 1.0) {
            currentLoad = newLoad;
            assignedProcesses.push_back(process);
            runningProcess = std::make_shared<Process>(process);
            return true;
        }
        return false;
    }

void FogNode::releaseProcess(int processID) {
    for (auto it = assignedProcesses.begin(); it != assignedProcesses.end(); ++it) {
        if (it->getProcessID() == processID) {
            auto resources = it->getRequiredResources();
            currentLoad -= (resources.cpu / processingPower);  // Use .cpu
            assignedProcesses.erase(it);
            if (runningProcess && runningProcess->getProcessID() == processID) {
                runningProcess = nullptr;
            }
            return;
        }
    }
}

void FogNode::printNodeInfo() const {
    std::cout << "Fog Node ID: " << nodeID
              << ", CPU: " << processingPower
              << ", Memory: " << memory
              << ", Bandwidth: " << bandwidth
              << ", Load: " << currentLoad
              << ", Delay: " << delay
              << ", Packet Loss: " << packetLoss
              << ", Location: " << location
              << ", Active: " << (isActive ? "Yes" : "No")
              << ", Assigned Processes: " << assignedProcesses.size()
              << std::endl;
}