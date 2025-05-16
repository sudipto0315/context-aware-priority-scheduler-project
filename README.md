🚀 How to Compile & Run
1️⃣ Compile the Code
❯ g++ -std=c++17 -o output/scheduler_simulation \
    src/main.cpp \
    src/simulation/Simulation.cpp \
    src/utils/Logger.cpp \
    src/schedulers/BaseScheduler.cpp \
    src/schedulers/ContextAwareScheduler.cpp \
    src/schedulers/FCFS.cpp \
    src/schedulers/SJF.cpp \
    src/models/Process.cpp \
    src/models/FogNode.cpp \
    -I. -I/opt/homebrew/include

2️⃣ Run the Simulation
❯ ./output/scheduler_simulation


🚀 Running the Tests
1️⃣ Install Google Test (if not installed)
brew install googletest         # macOS

2️⃣ Compile the Tests
❯ clang++ -std=c++17 -I/opt/homebrew/opt/googletest/include \
    -L/opt/homebrew/opt/googletest/lib \
    tests/test_process.cpp src/models/Process.cpp \
    -lgtest -lgtest_main -pthread -o output/test_process

❯ clang++ -std=c++17 -I/opt/homebrew/opt/googletest/include \
    -L/opt/homebrew/opt/googletest/lib \
    -lgtest -lgtest_main -pthread -o output/test_scheduler \
    tests/test_scheduler.cpp \
    src/schedulers/BaseScheduler.cpp \
    src/schedulers/ContextAwareScheduler.cpp \
    src/schedulers/FCFS.cpp \
    src/schedulers/SJF.cpp \
    src/models/Process.cpp \
    src/models/FogNode.cpp
    
❯ clang++ -std=c++17 \
    -I/opt/homebrew/opt/googletest/include \
    -L/opt/homebrew/opt/googletest/lib \
    -I/opt/homebrew/opt/nlohmann-json/include \
    -lgtest -lgtest_main -pthread -o output/test_simulation \
    tests/test_simulation.cpp \
    src/simulation/Simulation.cpp \
    src/utils/Logger.cpp \
    src/schedulers/BaseScheduler.cpp \
    src/schedulers/ContextAwareScheduler.cpp \
    src/schedulers/FCFS.cpp \
    src/schedulers/SJF.cpp \
    src/models/Process.cpp \
    src/models/FogNode.cpp


3️⃣ Run the Tests
❯ ./output/test_process
❯ ./output/test_scheduler
❯ ./output/test_simulation



🚀 How to Use the Makefile
1️⃣ Build the Project
make

2️⃣ Run the Scheduler Simulation
make run

3️⃣ Run Unit Tests
make test

4️⃣ Run only process tests
make testprocess

5️⃣ Run only scheduler tests
make testscheduler

6️⃣ Run only simulation tests
make testsimulation

7️⃣ Clean Compiled Files
make clean

8️⃣ Display Help
make help