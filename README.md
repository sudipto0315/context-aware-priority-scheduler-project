


          
# Context-Aware Priority Scheduler Project

This project implements a context-aware priority scheduler for fog computing environments. The scheduler intelligently allocates resources based on process context and priority.

## 📋 Project Overview

The scheduler simulation includes:
- Multiple scheduling algorithms (FCFS, SJF, Context-Aware)
- Process and fog node modeling
- Comprehensive testing suite
- Performance metrics and logging

## 🚀 How to Compile & Run

### 1️⃣ Compile the Code
```bash
g++ -std=c++17 -o output/scheduler_simulation \
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
```

### 2️⃣ Run the Simulation
```bash
./output/scheduler_simulation
```

## 🧪 Running the Tests

### 1️⃣ Install Google Test (if not installed)
```bash
brew install googletest         # macOS
```

### 2️⃣ Compile the Tests

#### Process Tests
```bash
clang++ -std=c++17 -I/opt/homebrew/opt/googletest/include \
    -L/opt/homebrew/opt/googletest/lib \
    tests/test_process.cpp src/models/Process.cpp \
    -lgtest -lgtest_main -pthread -o output/test_process
```

#### Scheduler Tests
```bash
clang++ -std=c++17 -I/opt/homebrew/opt/googletest/include \
    -L/opt/homebrew/opt/googletest/lib \
    -lgtest -lgtest_main -pthread -o output/test_scheduler \
    tests/test_scheduler.cpp \
    src/schedulers/BaseScheduler.cpp \
    src/schedulers/ContextAwareScheduler.cpp \
    src/schedulers/FCFS.cpp \
    src/schedulers/SJF.cpp \
    src/models/Process.cpp \
    src/models/FogNode.cpp
```

#### Simulation Tests
```bash
clang++ -std=c++17 \
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
```

### 3️⃣ Run the Tests
```bash
./output/test_process
./output/test_scheduler
./output/test_simulation
```

## 🛠️ Using the Makefile

The project includes a Makefile for simplified building and testing:

| Command | Description |
|---------|-------------|
| `make` | Build the entire project |
| `make run` | Run the scheduler simulation |
| `make test` | Run all unit tests |
| `make testprocess` | Run only process tests |
| `make testscheduler` | Run only scheduler tests |
| `make testsimulation` | Run only simulation tests |
| `make clean` | Clean compiled files |
| `make help` | Display help information |

## 📁 Project Structure

```
.
├── src/
│   ├── main.cpp
│   ├── models/
│   │   ├── Process.cpp
│   │   └── FogNode.cpp
│   ├── schedulers/
│   │   ├── BaseScheduler.cpp
│   │   ├── ContextAwareScheduler.cpp
│   │   ├── FCFS.cpp
│   │   └── SJF.cpp
│   ├── simulation/
│   │   └── Simulation.cpp
│   └── utils/
│       └── Logger.cpp
├── tests/
│   ├── test_process.cpp
│   ├── test_scheduler.cpp
│   └── test_simulation.cpp
├── output/
├── Makefile
└── README.md
```