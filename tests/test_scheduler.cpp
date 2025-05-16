#include "../src/schedulers/ContextAwareScheduler.h"
#include "../src/schedulers/FCFS.h"
#include "../src/schedulers/SJF.h"
#include "../src/models/Process.h"
#include "../src/models/FogNode.h"
#include <gtest/gtest.h>

// Test Fixture for Scheduler Tests
class SchedulerTest : public ::testing::Test {
protected:
    std::vector<FogNode> nodes;

    void SetUp() override {
        // Initialize fog nodes similar to config.json
        nodes.emplace_back(1, 16, 25, 202, 0.17, 10.33, 0.0005, "Zone_D", true);
        nodes.emplace_back(2, 11, 17, 305, 0.24, 4.49, 0.0004, "Zone_B", true);
    }
};

// Test FCFS Scheduler
TEST_F(SchedulerTest, FCFSSchedulesProcesses) {
    FCFSScheduler scheduler(nodes);
    std::shared_ptr<Process> p1 = std::make_shared<Process>(
        1, 0, 5, 1, 0, 0.0, 0.0, "none", 0.0, "generic", 0.0, 0.0, "Zone_D", std::time(nullptr),
        44.0, 0.04, 90.0, 0.0, 1.63, 0.66, 765);
    std::shared_ptr<Process> p2 = std::make_shared<Process>(
        2, 1, 3, 2, 0, 0.0, 0.0, "none", 0.0, "generic", 0.0, 0.0, "Zone_D", std::time(nullptr),
        10.0, 0.0624, 100.0, 0.0, 0.2, 2.73, 1060);
    scheduler.addProcess(p1);
    scheduler.addProcess(p2);
    EXPECT_NO_THROW(scheduler.schedule());
    // Verify p1 is processed first (FCFS)
    auto nextProcess = scheduler.getNextProcess();
    EXPECT_EQ(nextProcess, nullptr); // Queue should be empty after schedule()
}

// Test SJF Scheduler
TEST_F(SchedulerTest, SJFSchedulesProcesses) {
    SJFScheduler scheduler(nodes);
    std::shared_ptr<Process> p1 = std::make_shared<Process>(
        1, 0, 10, 1, 0, 0.0, 0.0, "none", 0.0, "generic", 0.0, 0.0, "Zone_D", std::time(nullptr),
        44.0, 0.04, 90.0, 0.0, 1.63, 0.66, 765);
    std::shared_ptr<Process> p2 = std::make_shared<Process>(
        2, 0, 5, 2, 0, 0.0, 0.0, "none", 0.0, "generic", 0.0, 0.0, "Zone_D", std::time(nullptr),
        10.0, 0.0624, 100.0, 0.0, 0.2, 2.73, 1060);
    scheduler.addProcess(p1);
    scheduler.addProcess(p2);
    EXPECT_NO_THROW(scheduler.schedule());
    // Verify p2 is processed first (shorter burst time)
    auto nextProcess = scheduler.getNextProcess();
    EXPECT_EQ(nextProcess, nullptr); // Queue should be empty after schedule()
}

// Test ContextAwareScheduler
TEST_F(SchedulerTest, ContextAwareSchedulesProcesses) {
    ContextAwareScheduler scheduler(nodes);
    std::shared_ptr<Process> p1 = std::make_shared<Process>(
        1, 0, 5, 1, 0, 0.0, 0.0, "none", 0.0, "generic", 0.0, 0.0, "Zone_D", std::time(nullptr),
        44.0, 0.04, 90.0, 0.0, 1.63, 0.66, 765);
    scheduler.addProcess(p1);
    EXPECT_NO_THROW(scheduler.schedule());
    auto nextProcess = scheduler.getNextProcess();
    EXPECT_EQ(nextProcess, nullptr); // Queue should be empty after schedule()
}

// Run all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}