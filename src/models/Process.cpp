#include "Process.h"
#include <iostream>

Process::Process(int id, int arrival, int burst, int prio,
                 int uid, double mob, double rp, std::string uh, double nps_score,
                 std::string appType, double latSens, double taskLoad,
                 std::string loc, std::time_t rt,
                 double bw, double pl, double delay,
                 double batt, double cpuRes, double memRes, int ds) {
    this->processID = id;
    this->arrivalTime = arrival;
    this->burstTime = burst;
    this->priority = prio;
    this->remainingTime = burst;
    this->userID = uid;
    this->mobility = mob;
    this->relinquishProbability = rp;
    this->usageHistory = uh;
    this->nps = nps_score;
    this->applicationType = appType;
    this->latencySensitivity = latSens;
    this->currentTaskLoad = taskLoad;
    this->requestLocation = loc;
    this->requestTime = rt;
    this->requiredBandwidth = bw;
    this->maxPacketLoss = pl;
    this->maxDelay = delay;
    this->batteryLifetime = batt;
    this->requiredResources = Resource(cpuRes, memRes); // Updated to use Resource struct
    this->dataSize = ds;

    // Basic validation
    if (this->mobility < 0.0 || this->mobility > 1.0) this->mobility = 0.0;
    if (this->relinquishProbability < 0.0 || this->relinquishProbability > 1.0) this->relinquishProbability = 0.0;
    if (this->latencySensitivity < 0.0 || this->latencySensitivity > 1.0) this->latencySensitivity = 0.0;
}

Process::Process(int id, int arrival, int burst, int prio) {
    this->processID = id;
    this->arrivalTime = arrival;
    this->burstTime = burst;
    this->priority = prio;
    this->remainingTime = burst;
    this->userID = 0;
    this->mobility = 0.0;
    this->relinquishProbability = 0.0;
    this->usageHistory = "none";
    this->nps = 0.0;
    this->applicationType = "generic";
    this->latencySensitivity = 0.0;
    this->currentTaskLoad = 0.0;
    this->requestLocation = "unknown";
    this->requestTime = std::time(nullptr);
    this->requiredBandwidth = 0.0;
    this->maxPacketLoss = 0.0;
    this->maxDelay = 0.0;
    this->batteryLifetime = 0.0;
    this->requiredResources = Resource(0.0, 0.0); // Updated to use Resource struct
    this->dataSize = 0;
}

void Process::decreaseRemainingTime() {
    if (this->remainingTime > 0) {
        --this->remainingTime;
    }
}

bool Process::isCompleted() const {
    return this->remainingTime == 0;
}

void Process::printProcessInfo() const {
    std::cout << "Process ID: " << this->processID 
              << ", Priority: " << this->priority 
              << ", Remaining Time: " << this->remainingTime 
              << ", Burst Time: " << this->burstTime 
              << ", Arrival Time: " << this->arrivalTime 
              << ", User ID: " << this->userID 
              << ", Mobility: " << this->mobility 
              << ", Location: " << this->requestLocation 
              << ", Application: " << this->applicationType 
              << ", Latency Sensitivity: " << this->latencySensitivity 
              << ", Bandwidth: " << this->requiredBandwidth << " Mbps"
              << ", CPU Required: " << this->requiredResources.cpu
              << ", Memory Required: " << this->requiredResources.memory
              << std::endl;
}