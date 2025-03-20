#include "Process.h"
#include <iostream>

// Constructor implementation
Process::Process(int id, int arrival, int burst, int prio,
    int uid, double mob, double rp, std::string uh, double nps_score,
    std::string appType, double latSens, double taskLoad,
    std::string loc, std::time_t rt,
    double bw, double pl, double delay,
    double batt, double cpuRes, double memRes, int ds)
    : processID(id), 
      arrivalTime(arrival), 
      burstTime(burst), 
      priority(prio),
      remainingTime(burst),
      processScore(0.0),  // Initialize score to 0.0
      userID(uid),
      mobility(mob),
      relinquishProbability(rp),
      usageHistory(uh),
      nps(nps_score),
      applicationType(appType),
      latencySensitivity(latSens),
      currentTaskLoad(taskLoad),
      requestLocation(loc),
      requestTime(rt),
      requiredBandwidth(bw),
      maxPacketLoss(pl),
      maxDelay(delay),
      batteryLifetime(batt),
      requiredResources(cpuRes, memRes),
      dataSize(ds) {}

// Decrease remaining time by 1
void Process::decreaseRemainingTime() {
    if (remainingTime > 0) {
        remainingTime--;
    }
}

// Check if process is completed
bool Process::isCompleted() const {
    return remainingTime <= 0;
}

// Print process information
void Process::printProcessInfo() const {
    std::cout << "Process ID: " << processID << std::endl;
    std::cout << "Arrival Time: " << arrivalTime << std::endl;
    std::cout << "Burst Time: " << burstTime << std::endl;
    std::cout << "Priority: " << priority << std::endl;
    std::cout << "Remaining Time: " << remainingTime << std::endl;
    std::cout << "Process Score: " << processScore << std::endl;  // Added score to printout
    std::cout << "Location: " << requestLocation << std::endl;
    std::cout << "Required CPU: " << requiredResources.cpu << ", Memory: " << requiredResources.memory << std::endl;
    std::cout << "Bandwidth: " << requiredBandwidth << " Mbps" << std::endl;
    std::cout << "Max Delay: " << maxDelay << " ms" << std::endl;
    std::cout << "Max Packet Loss: " << maxPacketLoss << std::endl;
    std::cout << "Latency Sensitivity: " << latencySensitivity << std::endl;
}