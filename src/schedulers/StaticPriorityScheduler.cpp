#include "StaticPriorityScheduler.h"
#include <algorithm>
#include <iostream>

StaticPriorityScheduler::StaticPriorityScheduler() {}

void StaticPriorityScheduler::schedule() {
    std::cout << "Starting Static Priority Scheduling..." << std::endl;

    if (processQueue.empty()) {
        std::cout << "No processes to schedule." << std::endl;
        return;
    }

    // Sort processes based on their static priority (lower value = higher priority)
    std::sort(processQueue.begin(), processQueue.end(), [](const std::shared_ptr<Process>& a, const std::shared_ptr<Process>& b) {
        return a->getPriority() < b->getPriority();
    });

    // Execute processes in order
    for (auto& process : processQueue) {
        std::cout << "Executing Process ID: " << process->getProcessID() << " with Priority: " << process->getPriority() << std::endl;
        process->printProcessInfo();
    }

    // Note: We might not want to clear the queue here if getNextProcess() is managing it
    // processQueue.clear(); // Comment out or adjust based on your design
}

std::shared_ptr<Process> StaticPriorityScheduler::getNextProcess() {
    if (processQueue.empty()) {
        return nullptr; // No process to return
    }

    // Ensure the queue is sorted only once (based on static priority)
    static bool isSorted = false;
    if (!isSorted) {
        std::sort(processQueue.begin(), processQueue.end(), [](const std::shared_ptr<Process>& a, const std::shared_ptr<Process>& b) {
            return a->getPriority() < b->getPriority();
        });
        isSorted = true;
    }

    // Get the next process (front of the sorted queue, highest priority)
    auto nextProcess = processQueue.front();
    processQueue.erase(processQueue.begin()); // Remove it from the queue
    return nextProcess;
}