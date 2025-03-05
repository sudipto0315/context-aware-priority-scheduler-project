#ifndef SIMULATION_H
#define SIMULATION_H

#include "../schedulers/BaseScheduler.h"
#include "../models/Process.h"
#include "../models/FogNode.h"
#include <memory>
#include <string>
#include <vector>

class Simulation {
private:
    std::unique_ptr<BaseScheduler> scheduler; // Pointer to selected scheduler
    std::vector<FogNode> fogNodes; // Available fog nodes in the simulation
    std::vector<Process> processList; // List of processes to schedule

    void loadConfig(const std::string& configFile); // Loads config.json

public:
    explicit Simulation(const std::string& configFile); // Constructor
    void run();  // Runs the simulation
    ~Simulation(); // Destructor
};

#endif // SIMULATION_H