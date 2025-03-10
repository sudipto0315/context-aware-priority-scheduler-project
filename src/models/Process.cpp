#include "Process.h"
#include <iostream>

// Constructor definition
Process::Process(int id, int arrival, int burst, int prio,
                 int uid, double mob, double rp, std::string uh, double nps_score,
                 std::string appType, double latSens, double taskLoad,
                 std::string loc, std::time_t rt,
                 double bw, double pl, double delay,
                 double batt, double cpuRes, double memRes, int ds)
    : processID(id), arrivalTime(arrival), burstTime(burst), priority(prio), remainingTime(burst),
      userID(uid), mobility(mob), relinquishProbability(rp), usageHistory(uh), nps(nps_score),
      applicationType(appType), latencySensitivity(latSens), currentTaskLoad(taskLoad),
      requestLocation(loc), requestTime(rt), requiredBandwidth(bw), maxPacketLoss(pl),
      maxDelay(delay), batteryLifetime(batt), requiredResources(cpuRes, memRes), dataSize(ds) {
    
    // Basic validation
    if (mobility < 0.0 || mobility > 1.0) mobility = 0.0;
    if (relinquishProbability < 0.0 || relinquishProbability > 1.0) relinquishProbability = 0.0;
    if (latencySensitivity < 0.0 || latencySensitivity > 1.0) latencySensitivity = 0.0;
}

void Process::decreaseRemainingTime() {
    if (remainingTime > 0) {
        --remainingTime;
    }
}

bool Process::isCompleted() const {
    return remainingTime == 0;
}

void Process::printProcessInfo() const {
    std::cout << "Process ID: " << processID 
              << ", Priority: " << priority 
              << ", Remaining Time: " << remainingTime 
              << ", Burst Time: " << burstTime 
              << ", Arrival Time: " << arrivalTime 
              << ", User ID: " << userID 
              << ", Mobility: " << mobility 
              << ", Location: " << requestLocation 
              << ", Application: " << applicationType 
              << ", Latency Sensitivity: " << latencySensitivity 
              << ", Bandwidth: " << requiredBandwidth << " Mbps"
              << ", CPU Required: " << requiredResources.cpu
              << ", Memory Required: " << requiredResources.memory
              << std::endl;
}
