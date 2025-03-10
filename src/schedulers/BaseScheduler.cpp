#include "BaseScheduler.h"

void BaseScheduler::clearQueue() {
    std::cout << "Clearing all processes from the queue.\n";
    processQueue.clear();
}

void BaseScheduler::printQueue() const {
    if (processQueue.empty()) {
        std::cout << "Process queue is empty.\n";
        return;
    }
    std::cout << "Current process queue:\n";
    for (const auto& process : processQueue) {
        process->printProcessInfo();
    }
}
