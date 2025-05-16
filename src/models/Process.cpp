#include "Process.h"
#include <iostream>

// Constructor implementation
Process::Process(int id, std::time_t arrival, std::time_t burst, int prio,
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
      //   dataSize(ds) {}
      dataSize(ds) {
          // Validate ranges
          if (mobility < 0.0 || mobility > 1.0) {
              throw std::invalid_argument("Mobility must be between 0.0 and 1.0");
          }
          if (relinquishProbability < 0.0 || relinquishProbability > 1.0) {
              throw std::invalid_argument("Relinquish Probability must be between 0.0 and 1.0");
          }
          if (nps < 0.0 || nps > 10.0) {
              throw std::invalid_argument("NPS must be between 0.0 and 10.0");
          }
          if (latencySensitivity < 0.0 || latencySensitivity > 1.0) {
              throw std::invalid_argument("Latency Sensitivity must be between 0.0 and 1.0");
          }
          if (maxPacketLoss < 0.0 || maxPacketLoss > 1.0) {
              throw std::invalid_argument("Max Packet Loss must be between 0.0 and 1.0");
          }
          if (burstTime <= 0) {
              throw std::invalid_argument("Burst Time must be positive");
          }
          if (requiredBandwidth < 0.0) {
              throw std::invalid_argument("Required Bandwidth must be non-negative");
          }
          if (maxDelay < 0.0) {
              throw std::invalid_argument("Max Delay must be non-negative");
          }
          if (batteryLifetime < 0.0) {
              throw std::invalid_argument("Battery Lifetime must be non-negative");
          }
          if (requiredResources.cpu < 0.0 || requiredResources.memory < 0.0) {
              throw std::invalid_argument("Required Resources (CPU, Memory) must be non-negative");
          }
          if (dataSize < 0) {
              throw std::invalid_argument("Data Size must be non-negative");
          }
      }

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
    std::cout << "Process Score: " << processScore << std::endl;
    std::cout << "User ID: " << userID << std::endl;
    std::cout << "Mobility: " << mobility << std::endl;
    std::cout << "Relinquish Probability: " << relinquishProbability << std::endl;
    std::cout << "Usage History: " << usageHistory << std::endl;
    std::cout << "NPS: " << nps << std::endl;
    std::cout << "Application Type: " << applicationType << std::endl;
    std::cout << "Latency Sensitivity: " << latencySensitivity << std::endl;
    std::cout << "Current Task Load: " << currentTaskLoad << std::endl;
    std::cout << "Location: " << requestLocation << std::endl;
    std::cout << "Request Time: " << std::ctime(&requestTime);
    std::cout << "Required Bandwidth: " << requiredBandwidth << " Mbps" << std::endl;
    std::cout << "Max Delay: " << maxDelay << " ms" << std::endl;
    std::cout << "Max Packet Loss: " << maxPacketLoss << std::endl;
    std::cout << "Battery Lifetime: " << batteryLifetime << " hours" << std::endl;
    std::cout << "Required CPU: " << requiredResources.cpu << ", Memory: " << requiredResources.memory << std::endl;
    std::cout << "Data Size: " << dataSize << " MB" << std::endl;
}