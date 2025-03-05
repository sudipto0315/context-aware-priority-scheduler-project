#include "simulation/Simulation.h"
#include "utils/Logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&timeT), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

int main(int argc, char* argv[]) {
    std::string configFile = "src/simulation/config.json"; // Default config file

    // Allow custom config file via command-line argument
    if (argc > 1) {
        configFile = argv[1];
    }

    // Initialize Logger
    Logger::init("output/logs.txt");
    Logger::log("===== [" + getCurrentTimestamp() + "] Priority Scheduling Simulation Started =====");

    try {
        // Load and run the simulation
        Simulation sim(configFile);
        sim.run();
    } catch (const std::exception& e) {
        Logger::logError(std::string("Simulation Error: ") + e.what());
        std::cerr << "[ERROR] Simulation encountered an exception: " << e.what() << std::endl;
    } catch (...) {
        Logger::logError("Unknown Error: An unexpected error occurred in the simulation.");
        std::cerr << "[ERROR] Unknown exception occurred." << std::endl;
    }

    // Final log and cleanup
    Logger::log("===== [" + getCurrentTimestamp() + "] Simulation Completed Successfully =====");
    Logger::close();

    return 0;
}