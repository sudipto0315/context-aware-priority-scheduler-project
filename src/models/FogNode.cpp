#include "FogNode.h"
#include <iostream>

FogNode::FogNode(int id, double cpu, double mem, double bw, double load, double dly, double pl, std::string loc, bool active)
    : nodeID(id), processingPower(cpu), memory(mem), initialMemory(mem), bandwidth(bw), currentLoad(load),
      usedCpu(0.0), usedMemory(0.0), delay(dly), packetLoss(pl), location(loc), isActive(active), 
      runningProcess(nullptr) {}

bool FogNode::assignProcess(const Process& process) {
    auto resources = process.getRequiredResources();
    
    // Calculate required resources
    double requiredCpu = resources.cpu;
    double requiredMemory = resources.memory;
    
    // Calculate new values after assignment
    double newUsedCpu = usedCpu + requiredCpu;
    double newUsedMemory = usedMemory + requiredMemory;
    double newLoad = newUsedCpu / processingPower;  // Update load based on CPU utilization
    double newRemainingMemory = initialMemory - newUsedMemory;

    std::cout << "Attempting to assign Process " << process.getProcessID() << " to Node " << nodeID << "\n";
    std::cout << "Current CPU Usage: " << usedCpu << "/" << processingPower << ", New Usage: " << newUsedCpu << "/" << processingPower << "\n";
    std::cout << "Current Memory Usage: " << usedMemory << "/" << initialMemory << ", New Usage: " << newUsedMemory << "/" << initialMemory << "\n";
    std::cout << "Current Load: " << currentLoad << ", New Load: " << newLoad << "\n";

    if (newLoad <= 1.0 && newRemainingMemory >= 0) {
        // Update resource tracking
        usedCpu = newUsedCpu;
        usedMemory = newUsedMemory;
        currentLoad = newLoad;
        memory = newRemainingMemory;
        
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
            
            // Calculate resources to reclaim
            double cpuToReclaim = resources.cpu;
            double memoryToReclaim = resources.memory;
            
            std::cout << "Releasing Process " << processID << " from Node " << nodeID << "\n";
            std::cout << "Reclaiming CPU: " << cpuToReclaim << ", Memory: " << memoryToReclaim << "\n";

            // Update resource tracking
            usedCpu -= cpuToReclaim;
            usedMemory -= memoryToReclaim;
            currentLoad = usedCpu / processingPower;  // Update load based on CPU utilization
            memory += memoryToReclaim;
            
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
              << " (Used: " << usedCpu << ", "
              << (usedCpu / processingPower * 100) << "%)"
              << ", Memory: " << initialMemory
              << " (Used: " << usedMemory << ", "
              << (usedMemory / initialMemory * 100) << "%)"
              << ", Bandwidth: " << bandwidth
              << ", Load: " << currentLoad
              << ", Delay: " << delay
              << ", Packet Loss: " << packetLoss
              << ", Location: " << location
              << ", Active: " << (isActive ? "Yes" : "No")
              << ", Assigned Processes: " << assignedProcesses.size()
              << std::endl;
}