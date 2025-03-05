#include "DynamicPriorityScheduler.h"
#include <algorithm>
#include <iostream>

DynamicPriorityScheduler::DynamicPriorityScheduler() {}

void DynamicPriorityScheduler::schedule() {
    std::cout << "Starting Dynamic Priority Scheduling..." << std::endl;

    if (processQueue.empty()) {
        std::cout << "No processes to schedule." << std::endl;
        return;
    }

    // Update priorities based on context-awareness
    updatePriorities();

    // Sort based on dynamically updated priorities
    std::sort(processQueue.begin(), processQueue.end(), [](const std::shared_ptr<Process>& a, const std::shared_ptr<Process>& b) {
        return a->getPriority() < b->getPriority(); // Lower value = higher priority
    });

    // Execute processes in order
    for (auto& process : processQueue) {
        std::cout << "Executing Process ID: " << process->getProcessID() << std::endl;
        process->printProcessInfo();
    }

    // Note: We might not want to clear the queue here if getNextProcess() is managing it
    // processQueue.clear(); // Comment out or adjust based on your design
}

void DynamicPriorityScheduler::updatePriorities() {
    std::cout << "Updating process priorities based on context-aware factors..." << std::endl;

    for (auto& process : processQueue) {
        // Example: Adjust priority dynamically based on mobility and latency sensitivity
        int newPriority = process->getPriority();

        if (process->getMobility() > 0.7) {
            newPriority -= 2; // Higher priority for highly mobile users
        }

        if (process->getLatencySensitivity() > 0.8) {
            newPriority -= 3; // Higher priority for latency-sensitive applications
        }

        if (process->getBatteryLifetime() < 1.0) {
            newPriority -= 1; // Prioritize tasks when battery is low
        }

        process->setPriority(std::max(newPriority, 1)); // Ensure priority remains valid
    }
}

std::shared_ptr<Process> DynamicPriorityScheduler::getNextProcess() {
    if (processQueue.empty()) {
        return nullptr; // No process to return
    }

    // Update priorities dynamically each time (since priorities can change)
    updatePriorities();

    // Sort based on the updated priorities
    std::sort(processQueue.begin(), processQueue.end(), [](const std::shared_ptr<Process>& a, const std::shared_ptr<Process>& b) {
        return a->getPriority() < b->getPriority(); // Lower value = higher priority
    });

    // Get the next process (front of the sorted queue, highest priority)
    auto nextProcess = processQueue.front();
    processQueue.erase(processQueue.begin()); // Remove it from the queue
    return nextProcess;
}