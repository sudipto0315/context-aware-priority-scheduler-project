# Compiler & Flags
CXX = g++
CLANGXX = clang++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread
GTEST_FLAGS = -lgtest -lgtest_main
INCLUDES = -I. -I/opt/homebrew/include -L/opt/homebrew/lib -lglpk
GTEST_INCLUDES = -I/opt/homebrew/opt/googletest/include -L/opt/homebrew/opt/googletest/lib
JSON_INCLUDES = -I/opt/homebrew/opt/nlohmann-json/include

# Directories
SRC_DIR = src
TEST_DIR = tests
OUTPUT_DIR = output
SIMULATION_DIR = src/simulation

# Source Files
SRC_FILES = $(SRC_DIR)/main.cpp \
            $(SRC_DIR)/simulation/Simulation.cpp \
            $(SRC_DIR)/utils/Logger.cpp \
            $(SRC_DIR)/schedulers/BaseScheduler.cpp \
            $(SRC_DIR)/schedulers/ContextAwareScheduler.cpp \
            $(SRC_DIR)/schedulers/FCFS.cpp \
            $(SRC_DIR)/schedulers/SJF.cpp \
            $(SRC_DIR)/models/Process.cpp \
            $(SRC_DIR)/models/FogNode.cpp

# Test Files
TEST_PROCESS_FILES = $(TEST_DIR)/test_process.cpp \
                     $(SRC_DIR)/models/Process.cpp

TEST_SCHEDULER_FILES = $(TEST_DIR)/test_scheduler.cpp \
                       $(SRC_DIR)/schedulers/BaseScheduler.cpp \
                       $(SRC_DIR)/schedulers/ContextAwareScheduler.cpp \
                       $(SRC_DIR)/schedulers/FCFS.cpp \
                       $(SRC_DIR)/schedulers/SJF.cpp \
                       $(SRC_DIR)/models/Process.cpp \
                       $(SRC_DIR)/models/FogNode.cpp

TEST_SIMULATION_FILES = $(TEST_DIR)/test_simulation.cpp \
                        $(SRC_DIR)/simulation/Simulation.cpp \
                        $(SRC_DIR)/utils/Logger.cpp \
                        $(SRC_DIR)/schedulers/BaseScheduler.cpp \
                        $(SRC_DIR)/schedulers/ContextAwareScheduler.cpp \
                        $(SRC_DIR)/schedulers/FCFS.cpp \
                        $(SRC_DIR)/schedulers/SJF.cpp \
                        $(SRC_DIR)/models/Process.cpp \
                        $(SRC_DIR)/models/FogNode.cpp

# Output Binaries
TARGET = $(OUTPUT_DIR)/scheduler_simulation
TEST_TARGETS = $(OUTPUT_DIR)/test_process $(OUTPUT_DIR)/test_scheduler $(OUTPUT_DIR)/test_simulation

# Default Target (Build Everything)
all: $(OUTPUT_DIR) $(TARGET) test

# Create output directory if it doesn't exist
$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

# Build the main scheduler simulation
$(TARGET): $(SRC_FILES) | $(OUTPUT_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

# Ensure logs.txt can be created
$(OUTPUT_DIR)/logs.txt: | $(OUTPUT_DIR)
	touch $@

# Build and run tests
test: $(TEST_TARGETS)
	@echo "Running Tests..."
	@for test in $(TEST_TARGETS); do if [ -x "$test" ]; then $test; fi; done

# Individual test targets
testprocess: $(OUTPUT_DIR)/test_process | $(OUTPUT_DIR)
	@echo "Running Process Tests..."
	@if [ -x "$(OUTPUT_DIR)/test_process" ]; then $(OUTPUT_DIR)/test_process; fi

testscheduler: $(OUTPUT_DIR)/test_scheduler | $(OUTPUT_DIR)
	@echo "Running Scheduler Tests..."
	@if [ -x "$(OUTPUT_DIR)/test_scheduler" ]; then $(OUTPUT_DIR)/test_scheduler; fi

testsimulation: $(OUTPUT_DIR)/test_simulation | $(OUTPUT_DIR)
	@echo "Running Simulation Tests..."
	@if [ -x "$(OUTPUT_DIR)/test_simulation" ]; then $(OUTPUT_DIR)/test_simulation; fi

# Test Executables
$(OUTPUT_DIR)/test_process: $(TEST_PROCESS_FILES) | $(OUTPUT_DIR)
	$(CLANGXX) $(CXXFLAGS) $(GTEST_INCLUDES) $^ -o $@ $(GTEST_FLAGS)

$(OUTPUT_DIR)/test_scheduler: $(TEST_SCHEDULER_FILES) | $(OUTPUT_DIR)
	$(CLANGXX) $(CXXFLAGS) $(GTEST_INCLUDES) $^ -o $@ $(GTEST_FLAGS)

$(OUTPUT_DIR)/test_simulation: $(TEST_SIMULATION_FILES) | $(OUTPUT_DIR)
	$(CLANGXX) $(CXXFLAGS) $(GTEST_INCLUDES) $(JSON_INCLUDES) $^ -o $@ $(GTEST_FLAGS)

# Clean compiled files
clean:
	rm -rf $(OUTPUT_DIR)/*.o $(OUTPUT_DIR)/*.out $(OUTPUT_DIR)/test_* $(TARGET)

# Run the simulation (ensures required files exist)
run: $(TARGET) $(OUTPUT_DIR)/logs.txt
	./$(TARGET)

# Help command
help:
	@echo "Makefile for Priority-Based Scheduling"
	@echo "Usage:"
	@echo "  make            - Build the project"
	@echo "  make run        - Run the simulation"
	@echo "  make test          - Build and run all tests"
	@echo "  make testprocess   - Build and run only process tests"
	@echo "  make testscheduler - Build and run only scheduler tests"
	@echo "  make testsimulation - Build and run only simulation tests"
	@echo "  make clean      - Remove compiled files"