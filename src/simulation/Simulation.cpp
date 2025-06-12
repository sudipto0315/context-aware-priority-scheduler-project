#include "Simulation.h"
#include "../schedulers/ContextAwareScheduler.h"
#include "../schedulers/FCFS.h"
#include "../schedulers/SJF.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>
#include <typeinfo>

using json = nlohmann::json;

Simulation::Simulation(const std::string& configFile) {
    std::cout << "Initializing Simulation with config: " << configFile << "\n";
    loadConfig(configFile);
}

void Simulation::loadConfig(const std::string& configFile) {
    std::ifstream file(configFile);
    if (!file) {
        std::cerr << "Error: Unable to open config file: " << configFile << "\n";
        exit(EXIT_FAILURE);
    }

    json config;
    file >> config;

    // Step 1: Load fog nodes first to ensure the vector is populated
    if (config.contains("fog_nodes")) {
        for (const auto& nodeData : config["fog_nodes"]) {
            fogNodes.emplace_back(FogNode(
                nodeData["id"],
                nodeData["cpu_capacity"],
                nodeData["memory"],
                nodeData["network_bandwidth"],
                nodeData.value("current_load", 0.0),
                nodeData.value("delay", 5.0),
                nodeData.value("packet_loss", 0.0),
                nodeData.value("location", "unknown"),
                nodeData.value("is_active", true)
            ));
        }
    }

    // Step 2: Create the scheduler with the populated fogNodes
    std::string schedulerType = config["scheduler"];
    if (schedulerType == "ContextAware") {
        scheduler = std::make_unique<ContextAwareScheduler>(fogNodes);
    } else if (schedulerType == "FCFS") {
        scheduler = std::make_unique<FCFSScheduler>(fogNodes);
    } else if (schedulerType == "SJF") {
        scheduler = std::make_unique<SJFScheduler>(fogNodes);
    } else {
        std::cerr << "Error: Unknown scheduler type in config file.\n";
        exit(EXIT_FAILURE);
    }

    // Step 3: Load processes
    for (const auto& processData : config["processes"]) {
        std::string usageHistoryStr = processData["usage_history"].get<std::string>();

        json resources = processData.value("required_resources", json::object());
        int required_cpu = resources.value("cpu", 0);
        int required_memory = resources.value("memory", 0);
        Process process(
            processData["id"].get<int>(),
            processData["arrival_time"].get<std::time_t>(),
            processData["burst_time"].get<std::time_t>(),
            processData["priority"].get<int>(),
            processData["user_id"].get<int>(),
            processData["mobility"].get<double>(),
            processData["relinquish_probability"].get<double>(),
            usageHistoryStr,
            processData["nps"].get<double>(),
            processData["application_type"].get<std::string>(),
            processData["latency_sensitivity"].get<double>(),
            processData["current_task_load"].get<double>(),
            processData["request_location"].get<std::string>(),
            processData["request_time"].get<std::time_t>(),
            processData["required_bandwidth"].get<double>(),
            processData["max_packet_loss"].get<double>(),
            processData["max_delay"].get<double>(),
            processData["battery_lifetime"].get<double>(),
            required_cpu,          // Separate CPU resource
            required_memory,       // Separate memory resource
            processData["data_size"].get<int>()
        );

        processList.push_back(process);
    }
}

void Simulation::run() {
    std::cout << "Starting Simulation...\n";
    for (auto& process : processList) {
        scheduler->addProcess(std::make_shared<Process>(process));
    }
    scheduler->printQueue();
    scheduler->schedule();
    scheduler->printSchedulingSummary();
    scheduler->printSchedulingMetrics();
}

Simulation::~Simulation() = default;