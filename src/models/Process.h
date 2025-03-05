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
    int arrivalTime;       // Arrival time of the process
    int burstTime;         // Total burst time required by the process
    int priority;          // Base priority level (lower value = higher priority)
    int remainingTime;     // Remaining execution time (for preemptive scheduling)

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
    // Full constructor with all attributes
    Process(int id, int arrival, int burst, int prio,
            int uid, double mob, double rp, std::string uh, double nps_score,
            std::string appType, double latSens, double taskLoad,
            std::string loc, std::time_t rt,
            double bw, double pl, double delay,
            double batt, double cpuRes, double memRes, int ds);

    // Legacy constructor (backward compatibility)
    Process(int id, int arrival, int burst, int prio);

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
    Resource getRequiredResources() const { return requiredResources; } // Updated return type
    int getDataSize() const { return dataSize; }
    int getExecutionTime() const { return burstTime; }
    int getDeadline() const { return arrivalTime + burstTime; }

    // Setters
    void setPriority(int newPriority) { priority = newPriority; }
    void setRemainingTime(int time) { remainingTime = time; }
    void setMobility(double mob) { mobility = mob; }
    void setBatteryLifetime(double batt) { batteryLifetime = batt; }
    void setCurrentTaskLoad(double load) { currentTaskLoad = load; }
    void setRequiredResources(double cpu, double memory) { requiredResources = Resource(cpu, memory); }

    // Utility Functions
    void decreaseRemainingTime();
    bool isCompleted() const;
    void printProcessInfo() const;
};

#endif // PROCESS_H