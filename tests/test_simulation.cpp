#include "../src/simulation/Simulation.h"
#include "../src/utils/Logger.h"
#include <gtest/gtest.h>
// Test Simulation Execution
TEST(SimulationTest, RunsSuccessfully) { 
    Logger::init("output/test_logs.txt"); // Separate log file for tests

    Simulation sim("src/simulation/config.json"); // Use test config
    EXPECT_NO_THROW(sim.run()); // Ensure simulation runs without crashing

    Logger::close();
}

// Run all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
