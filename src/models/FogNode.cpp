#include "FogNode.h"
#include <iostream>

FogNode::FogNode(int id, double cpu, double mem, double bw, double load, double dly, double pl, std::string loc, bool active)
    : nodeID(id), processingPower(cpu), memory(mem), bandwidth(bw), currentLoad(load),
      delay(dly), packetLoss(pl), location(loc), isActive(active), runningProcess(nullptr) {}

bool FogNode::assignProcess(const Process& process) {
    auto resources = process.getRequiredResources();
    
    double newLoad = currentLoad + (resources.cpu / processingPower);
    double remainingMemory = memory - resources.memory; 

    std::cout << "Attempting to assign Process " << process.getProcessID() << " to Node " << nodeID << "\n";
    std::cout << "Current Load: " << currentLoad << ", New Load: " << newLoad << "\n";
    std::cout << "Current Memory: " << memory << ", Required: " << resources.memory << ", Remaining: " << remainingMemory << "\n";

    if (newLoad <= 1.0 && remainingMemory >= 0) {
        currentLoad = newLoad;
        memory = remainingMemory;
        assignedProcesses.push_back(process);
        runningProcess = std::make_shared<Process>(process);

        std::cout << "Process " << process.getProcessID() << " assigned successfully.\n";
        return true;
    }

    std::cout << "Process " << process.getProcessID() << " assignment failed. Insufficient resources.\n";
    return false;
}

void FogNode::releaseProcess(int processID) {
    for (auto it = assignedProcesses.begin(); it != assignedProcesses.end(); ++it) {
        if (it->getProcessID() == processID) {
            auto resources = it->getRequiredResources();
            
            std::cout << "Releasing Process " << processID << " from Node " << nodeID << "\n";
            std::cout << "Reclaiming CPU: " << resources.cpu << ", Memory: " << resources.memory << "\n";

            currentLoad -= (resources.cpu / processingPower);
            memory += resources.memory;
            assignedProcesses.erase(it);

            if (runningProcess && runningProcess->getProcessID() == processID) {
                runningProcess = nullptr;
                std::cout << "Running process cleared.\n";
            }

            std::cout << "Process " << processID << " released successfully.\n";
            return;
        }
    }

    std::cout << "Process " << processID << " not found on Node " << nodeID << ".\n";
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
