#ifndef FOGNODE_H
#define FOGNODE_H

#include "Process.h"
#include <vector>
#include <string>
#include <memory>

class FogNode {
private:
    int nodeID;               // Unique identifier for the fog node
    double processingPower;   // Computational capacity (CPU units or operations/sec)
    double memory;            // Available memory (MB)
    double bandwidth;         // Available network bandwidth (Mbps)
    double currentLoad;       // Current utilization (0.0 to 1.0, where 1.0 is fully loaded)
    double delay;             // Network latency to this node (ms)
    double packetLoss;        // Current packet loss rate (0.0 to 1.0)
    std::string location;     // Physical or logical location (e.g., "zone_A")
    bool isActive;            // Operational status (online/offline)

    std::vector<Process> assignedProcesses; // List of assigned processes
    std::shared_ptr<Process> runningProcess; // Currently running process

public:
    // Constructor
    FogNode(int id, double cpu, double mem, double bw, double load, double dly, double pl, std::string loc, bool active);

    // Getters
    int getNodeID() const { return nodeID; }
    double getProcessingPower() const { return processingPower; }
    double getMemory() const { return memory; }
    double getBandwidth() const { return bandwidth; }
    double getCurrentLoad() const { return currentLoad; }
    double getDelay() const { return delay; }
    double getPacketLoss() const { return packetLoss; }
    std::string getLocation() const { return location; }
    bool getIsActive() const { return isActive; }
    int getAssignedProcessCount() const { return assignedProcesses.size(); }
    std::vector<Process> getAssignedProcesses() const { return assignedProcesses; }
    std::shared_ptr<Process> getRunningProcess() const { return runningProcess; }
    double getCpuCapacity() const { return processingPower; }

    // Setters
    void setProcessingPower(double cpu) { processingPower = cpu; }
    void setMemory(double mem) { memory = mem; }
    void setBandwidth(double bw) { bandwidth = bw; }
    void setCurrentLoad(double load) { currentLoad = load; }
    void setDelay(double dly) { delay = dly; }
    void setPacketLoss(double pl) { packetLoss = pl; }
    void setLocation(const std::string& loc) { location = loc; }
    void setIsActive(bool active) { isActive = active; }

    // Task allocation methods
    bool assignProcess(const Process& process);
    void releaseProcess(int processID);
    void printNodeInfo() const;
};

#endif // FOGNODE_H