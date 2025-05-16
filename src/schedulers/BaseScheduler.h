#ifndef BASESCHEDULER_H
#define BASESCHEDULER_H

#include "../models/Process.h"
#include <vector>
#include <memory>
#include <algorithm>
#include <iostream>

class BaseScheduler {
protected:
    std::vector<std::shared_ptr<Process>> processQueue; // Process queue

public:
    // Virtual destructor to ensure proper cleanup in derived classes
    virtual ~BaseScheduler() {}

    // Pure virtual function for scheduling (to be implemented by derived classes)
    virtual void schedule() = 0;

    // Pure virtual function for SchedulingSummary (to be implemented by derived classes)
    virtual void printSchedulingSummary() const = 0;

    // Pure virtual function for SchedulingMetrics (to be implemented by derived classes)
    virtual void printSchedulingMetrics() const = 0;

    // Add a process to the queue
    virtual void addProcess(std::shared_ptr<Process> process) {
        processQueue.push_back(process);
    }

    // Remove a process from the queue
    virtual void removeProcess(int processID) {
        processQueue.erase(std::remove_if(processQueue.begin(), processQueue.end(),
            [processID](std::shared_ptr<Process> p) { return p->getProcessID() == processID; }),
            processQueue.end());
    }

    // Clear all processes from the queue
    virtual void clearQueue();

    // Get the next process (to be implemented in derived classes)
    virtual std::shared_ptr<Process> getNextProcess() = 0;

    // Print the current process queue with logging
    virtual void printQueue() const;
};

#endif // BASESCHEDULER_H