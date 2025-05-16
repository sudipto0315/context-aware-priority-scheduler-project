#ifndef PROCESS_H
#define PROCESS_H

#include <string>
#include <ctime>

struct Resource {
    double cpu;    // CPU units required
    double memory; // Memory required (e.g., in MB)
    Resource(double c = 0.0, double m = 0.0) : cpu(c), memory(m) {}
};

class Process {
private:
    int processID;         // Unique identifier for the process
    std::time_t arrivalTime;       // Arrival time of the process
    std::time_t burstTime;         // Total burst time required by the process
    int priority;          // Base priority level (lower value = higher priority)
    std::time_t remainingTime;     // Remaining execution time (for preemptive scheduling)
    double processScore;   // Score used for process prioritization in schedulers

    // User Context
    int userID;            // Identifier for the user initiating the process
    double mobility;       // Mobility level (0.0 = stationary, 1.0 = highly mobile)
    double relinquishProbability; // Likelihood of user abandoning the process (0.0 to 1.0)
    std::string usageHistory;     // Simplified usage history (e.g., "high_frequency")
    double nps;            // Net Promoter Score (e.g., 0 to 10)

    // Application Context
    std::string applicationType;  // e.g., "computer_vision", "log_processing"
    double latencySensitivity;    // Sensitivity to latency (0.0 = low, 1.0 = high)
    double currentTaskLoad;       // Current computational load (arbitrary units)

    // Environmental Context
    std::string requestLocation;  // Location of request (e.g., "zone_A")
    std::time_t requestTime;      // Timestamp of the process request

    // Network Context
    double requiredBandwidth;     // Bandwidth requirement (Mbps)
    double maxPacketLoss;         // Maximum tolerable packet loss (0.0 to 1.0)
    double maxDelay;              // Maximum tolerable delay (ms)

    // Device Context
    double batteryLifetime;       // Remaining battery life (hours)
    Resource requiredResources;   // Required compute resources (CPU and memory)
    int dataSize;                 // Size of data to process (MB)

public:
    // Constructor declaration
    Process(int id, std::time_t arrival, std::time_t burst, int prio,
        int uid = 0, double mob = 0.0, double rp = 0.0, std::string uh = "none", double nps_score = 0.0,
        std::string appType = "generic", double latSens = 0.0, double taskLoad = 0.0,
        std::string loc = "unknown", std::time_t rt = std::time(nullptr),
        double bw = 0.0, double pl = 0.0, double delay = 0.0,
        double batt = 0.0, double cpuRes = 0.0, double memRes = 0.0, int ds = 0);

    // Getters
    int getProcessID() const { return processID; }
    int getArrivalTime() const { return arrivalTime; }
    int getBurstTime() const { return burstTime; }
    int getPriority() const { return priority; }
    int getRemainingTime() const { return remainingTime; }
    int getUserID() const { return userID; }
    double getMobility() const { return mobility; }
    double getRelinquishProbability() const { return relinquishProbability; }
    std::string getUsageHistory() const { return usageHistory; }
    double getNps() const { return nps; }
    std::string getApplicationType() const { return applicationType; }
    double getLatencySensitivity() const { return latencySensitivity; }
    double getCurrentTaskLoad() const { return currentTaskLoad; }
    std::string getRequestLocation() const { return requestLocation; }
    std::time_t getRequestTime() const { return requestTime; }
    double getRequiredBandwidth() const { return requiredBandwidth; }
    double getMaxPacketLoss() const { return maxPacketLoss; }
    double getMaxDelay() const { return maxDelay; }
    double getBatteryLifetime() const { return batteryLifetime; }
    Resource getRequiredResources() const { return requiredResources; }
    int getDataSize() const { return dataSize; }
    int getExecutionTime() const { return burstTime; }
    int getDeadline() const { return arrivalTime + burstTime; }
    double getProcessScore() const { return processScore; }  // New getter for score

    // Setters
    void setPriority(int newPriority) { priority = newPriority; }
    void setRemainingTime(int time) { remainingTime = time; }
    void setMobility(double mob) { mobility = mob; }
    void setBatteryLifetime(double batt) { batteryLifetime = batt; }
    void setCurrentTaskLoad(double load) { currentTaskLoad = load; }
    void setRequiredResources(double cpu, double memory) { requiredResources = Resource(cpu, memory); }
    void setRequiredBandwidth(double bw) { requiredBandwidth = bw; }
    void setProcessScore(double newProcessScore) { processScore = newProcessScore; }  // New setter for score

    // Utility Functions
    void decreaseRemainingTime();
    bool isCompleted() const;
    void printProcessInfo() const;
};

#endif // PROCESS_H