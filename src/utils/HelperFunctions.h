#ifndef HELPER_FUNCTIONS_H
#define HELPER_FUNCTIONS_H

#include "../models/Process.h"
#include <vector>
#include <algorithm>

namespace Utils {

    // Sorts processes by priority (lower value = higher priority)
    inline void sortByPriority(std::vector<Process>& processes) {
        std::sort(processes.begin(), processes.end(),
            [](const Process& a, const Process& b) {
                if (a.getPriority() == b.getPriority()) {
                    return a.getArrivalTime() < b.getArrivalTime(); // FIFO if same priority
                }
                return a.getPriority() < b.getPriority();
            });
    }

    // Sorts processes by arrival time (earlier arrival first)
    inline void sortByArrivalTime(std::vector<Process>& processes) {
        std::sort(processes.begin(), processes.end(),
            [](const Process& a, const Process& b) {
                return a.getArrivalTime() < b.getArrivalTime();
            });
    }

    // Checks if all processes are completed
    inline bool allProcessesCompleted(const std::vector<Process>& processes) {
        return std::all_of(processes.begin(), processes.end(),
            [](const Process& process) { return process.getRemainingTime() == 0; });
    }

    // Checks if any process is still running (not necessarily completed)
    inline bool isAnyProcessRunning(const std::vector<Process>& processes) {
        return std::any_of(processes.begin(), processes.end(),
            [](const Process& process) { return process.getRemainingTime() > 0; });
    }

}

#endif // HELPER_FUNCTIONS_H