#include "NonPreemptiveScheduler.h"
#include <algorithm>
#include <iostream>

NonPreemptiveScheduler::NonPreemptiveScheduler(const std::vector<FogNode>& nodes) : fogNodes(nodes) {}

void NonPreemptiveScheduler::schedule() {
    std::cout << "Starting Non-Preemptive Scheduling..." << std::endl;

    if (processQueue.empty()) {
        std::cout << "No processes to schedule." << std::endl;
        return;
    }

    // Sort by arrival time (and priority as tiebreaker)
    std::sort(processQueue.begin(), processQueue.end(), [](const std::shared_ptr<Process>& a, const std::shared_ptr<Process>& b) {
        return (a->getArrivalTime() < b->getArrivalTime()) ||
               (a->getArrivalTime() == b->getArrivalTime() && a->getPriority() < b->getPriority());
    });

    for (auto& process : processQueue) {
        std::cout << "Executing Process ID: " << process->getProcessID() << std::endl;
        process->printProcessInfo();
    }
}

std::shared_ptr<Process> NonPreemptiveScheduler::getNextProcess() {
    if (processQueue.empty()) {
        return nullptr;
    }

    // Ensure the queue is sorted only once
    static bool isSorted = false;
    if (!isSorted) {
        std::sort(processQueue.begin(), processQueue.end(), [](const std::shared_ptr<Process>& a, const std::shared_ptr<Process>& b) {
            return (a->getArrivalTime() < b->getArrivalTime()) ||
                   (a->getArrivalTime() == b->getArrivalTime() && a->getPriority() < b->getPriority());
        });
        isSorted = true;
    }

    auto nextProcess = processQueue.front();
    processQueue.erase(processQueue.begin()); 
    return nextProcess;
}