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
    std::string configFile = "src/simulation/config.json";

    if (argc > 1) {
        configFile = argv[1];
    }

    Logger::init("output/logs.txt");
    std::cout << "===== [" << getCurrentTimestamp() << "] Scheduling Simulation Started =====" << std::endl;

    try {
        Simulation sim(configFile);
        sim.run();
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Simulation encountered an exception: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "[ERROR] Unknown exception occurred." << std::endl;
    }

    std::cout << "===== [" << getCurrentTimestamp() << "] Scheduling Simulation Completed Successfully =====" << std::endl;
    Logger::close();

    return 0;
}
