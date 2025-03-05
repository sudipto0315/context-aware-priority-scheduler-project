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
}

// Test Process Execution (reduce remaining time)
TEST(ProcessTest, Execution) {
    Process p(2, 0, 10, 2);

    // Decrement remaining time by 3
    for (int i = 0; i < 3; i++) {
        p.decreaseRemainingTime();  // FIXED: execute() → decreaseRemainingTime()
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

// Run all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
