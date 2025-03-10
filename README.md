🚀 How to Compile & Run
1️⃣ Compile the Code
~/Developer/scheduler/context-aware-priority-scheduler-project
❯ g++ -std=c++17 -o output/scheduler_simulation \
    src/main.cpp \
    src/simulation/Simulation.cpp \
    src/utils/Logger.cpp \
    src/schedulers/BaseScheduler.cpp \
    src/schedulers/DynamicPriorityScheduler.cpp \
    src/schedulers/NonPreemptiveScheduler.cpp \
    src/schedulers/PreemptiveScheduler.cpp \
    src/schedulers/StaticPriorityScheduler.cpp \
    src/schedulers/ContextAwareScheduler.cpp \
    src/models/Process.cpp \
    src/models/FogNode.cpp \
    -I. -I/opt/homebrew/include

2️⃣ Run the Simulation
~/Developer/scheduler/context-aware-priority-scheduler-project
❯ ./output/scheduler_simulation


🚀 Running the Tests
1️⃣ Install Google Test (if not installed)
brew install googletest         # macOS

2️⃣ Compile the Tests
~/Developer/scheduler/priority-scheduler-project
❯ clang++ -std=c++17 -I/opt/homebrew/opt/googletest/include \
    -L/opt/homebrew/opt/googletest/lib \
    tests/test_process.cpp src/models/Process.cpp \
    -lgtest -lgtest_main -pthread -o test_process

❯ clang++ -std=c++17 -I/opt/homebrew/opt/googletest/include \
    -L/opt/homebrew/opt/googletest/lib \
    -lgtest -lgtest_main -pthread -o test_scheduler \
    tests/test_scheduler.cpp \
    src/schedulers/DynamicPriorityScheduler.cpp \
    src/schedulers/NonPreemptiveScheduler.cpp \
    src/schedulers/PreemptiveScheduler.cpp \
    src/schedulers/StaticPriorityScheduler.cpp \
    src/models/Process.cpp
    
❯ clang++ -std=c++17 \
    -I/opt/homebrew/opt/googletest/include \
    -L/opt/homebrew/opt/googletest/lib \
    -I/opt/homebrew/opt/nlohmann-json/include \
    -lgtest -lgtest_main -pthread -o test_simulation \
    tests/test_simulation.cpp \
    src/simulation/Simulation.cpp \
    src/utils/Logger.cpp \
    src/schedulers/DynamicPriorityScheduler.cpp \
    src/schedulers/NonPreemptiveScheduler.cpp \
    src/schedulers/PreemptiveScheduler.cpp \
    src/schedulers/StaticPriorityScheduler.cpp \
    src/models/Process.cpp


3️⃣ Run the Tests
~/Developer/scheduler/priority-scheduler-project/
❯ ./test_process
❯ ./test_scheduler
❯ ./test_simulation



🚀 How to Use the Makefile
1️⃣ Build the Project
make

2️⃣ Run the Scheduler Simulation
make run

3️⃣ Run Unit Tests
make test

4️⃣ Clean Compiled Files
make clean

5️⃣ Display Help
make help