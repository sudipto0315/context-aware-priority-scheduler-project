#include "../src/schedulers/PreemptiveScheduler.h"
#include "../src/schedulers/NonPreemptiveScheduler.h"
#include "../src/schedulers/StaticPriorityScheduler.h"
#include "../src/schedulers/DynamicPriorityScheduler.h"
#include "../src/models/Process.h"
#include <gtest/gtest.h>

// Test Preemptive Scheduler
TEST(SchedulerTest, PreemptiveExecution) {
    PreemptiveScheduler scheduler;

    scheduler.addProcess(Process(1, 0, 5, 2)); // Lower priority
    scheduler.addProcess(Process(2, 0, 3, 1)); // Higher priority (preempts Process 1)

    scheduler.schedule();

    EXPECT_EQ(scheduler.getExecutionOrder()[0], 2); // Process 2 should execute first
}

// Test Non-Preemptive Scheduler
TEST(SchedulerTest, NonPreemptiveExecution) {
    NonPreemptiveScheduler scheduler;

    scheduler.addProcess(Process(1, 0, 5, 2));
    scheduler.addProcess(Process(2, 1, 3, 1));

    scheduler.schedule();

    EXPECT_EQ(scheduler.getExecutionOrder()[0], 1); // Process 1 starts first (Non-preemptive)
}

// Test Static Priority Scheduler
TEST(SchedulerTest, StaticPriorityExecution) {
    StaticPriorityScheduler scheduler;

    scheduler.addProcess(Process(1, 0, 5, 2)); // Lower priority
    scheduler.addProcess(Process(2, 0, 3, 1)); // Higher priority

    scheduler.schedule();

    EXPECT_EQ(scheduler.getExecutionOrder()[0], 2); // Process 2 has higher priority and executes first
}

// Test Dynamic Priority Scheduler
TEST(SchedulerTest, DynamicPriorityExecution) {
    DynamicPriorityScheduler scheduler;

    scheduler.addProcess(Process(1, 0, 5, 2)); // Lower priority
    scheduler.addProcess(Process(2, 0, 3, 1)); // Higher priority initially

    scheduler.schedule();

    EXPECT_EQ(scheduler.getExecutionOrder()[0], 2); // Initially, Process 2 has higher priority
}

// Run all tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
