#include "../src/models/Process.h"
#include <gtest/gtest.h>

// Test Process Initialization
TEST(ProcessTest, Initialization) {
    Process p(1, 2, 5, 1); // id=1, arrival_time=2, burst_time=5, priority=1

    EXPECT_EQ(p.getProcessID(), 1);
    EXPECT_EQ(p.getArrivalTime(), 2);
    EXPECT_EQ(p.getBurstTime(), 5);
    EXPECT_EQ(p.getPriority(), 1);
    EXPECT_EQ(p.getRemainingTime(), 5); // Initially, remaining time = burst time
    EXPECT_EQ(p.getExecutionTime(), 5);
    EXPECT_EQ(p.getDeadline(), 7); // arrivalTime + burstTime = 2 + 5
}

// Test Process Execution (reduce remaining time)
TEST(ProcessTest, Execution) {
    Process p(2, 0, 10, 2);

    // Decrement remaining time by 3
    for (int i = 0; i < 3; i++) {
        p.decreaseRemainingTime();
    }
    EXPECT_EQ(p.getRemainingTime(), 7);

    // Decrement remaining time by 7
    for (int i = 0; i < 7; i++) {
        p.decreaseRemainingTime();
    }
    EXPECT_EQ(p.getRemainingTime(), 0);
}

// Test Completion Check
TEST(ProcessTest, CompletionCheck) {
    Process p(3, 1, 6, 3);
    EXPECT_FALSE(p.isCompleted());

    // Fully execute process
    for (int i = 0; i < 6; i++) {
        p.decreaseRemainingTime();
    }
    EXPECT_TRUE(p.isCompleted());
}

// Test Additional Getters
TEST(ProcessTest, ContextGetters) {
    Process p(4, 0, 8, 2, 100, 0.5, 0.1, "high_frequency", 8.0,
              "video_streaming", 0.7, 1.0, "Zone_A", std::time(nullptr),
              50.0, 0.01, 20.0, 10.0, 2.0, 4.0, 500);

    EXPECT_EQ(p.getUserID(), 100);
    EXPECT_DOUBLE_EQ(p.getMobility(), 0.5);
    EXPECT_DOUBLE_EQ(p.getRelinquishProbability(), 0.1);
    EXPECT_EQ(p.getUsageHistory(), "0.42,0.39,0.34");
    EXPECT_DOUBLE_EQ(p.getNps(), 8.0);
    EXPECT_EQ(p.getApplicationType(), "video_streaming");
    EXPECT_DOUBLE_EQ(p.getLatencySensitivity(), 0.7);
    EXPECT_DOUBLE_EQ(p.getCurrentTaskLoad(), 1.0);
    EXPECT_EQ(p.getRequestLocation(), "Zone_A");
    EXPECT_DOUBLE_EQ(p.getRequiredBandwidth(), 50.0);
    EXPECT_DOUBLE_EQ(p.getMaxPacketLoss(), 0.01);
    EXPECT_DOUBLE_EQ(p.getMaxDelay(), 20.0);
    EXPECT_DOUBLE_EQ(p.getBatteryLifetime(), 10.0);
    Resource res = p.getRequiredResources();
    EXPECT_DOUBLE_EQ(res.cpu, 2.0);
    EXPECT_DOUBLE_EQ(res.memory, 4.0);
    EXPECT_EQ(p.getDataSize(), 500);
}

// Run all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}