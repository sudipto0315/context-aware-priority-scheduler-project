# Compiler & Flags
CXX = g++
CLANGXX = clang++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread
GTEST_FLAGS = -lgtest -lgtest_main
INCLUDES = -I.. -I/opt/homebrew/include
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
            $(SRC_DIR)/schedulers/DynamicPriorityScheduler.cpp \
            $(SRC_DIR)/schedulers/NonPreemptiveScheduler.cpp \
            $(SRC_DIR)/schedulers/PreemptiveScheduler.cpp \
            $(SRC_DIR)/schedulers/StaticPriorityScheduler.cpp \
            $(SRC_DIR)/models/Process.cpp

# Test Files
TEST_PROCESS_FILES = $(TEST_DIR)/test_process.cpp $(SRC_DIR)/models/Process.cpp
TEST_SCHEDULER_FILES = $(TEST_DIR)/test_scheduler.cpp \
                       $(SRC_DIR)/schedulers/DynamicPriorityScheduler.cpp \
                       $(SRC_DIR)/schedulers/NonPreemptiveScheduler.cpp \
                       $(SRC_DIR)/schedulers/PreemptiveScheduler.cpp \
                       $(SRC_DIR)/schedulers/StaticPriorityScheduler.cpp \
                       $(SRC_DIR)/models/Process.cpp

TEST_SIMULATION_FILES = $(TEST_DIR)/test_simulation.cpp \
                        $(SRC_DIR)/simulation/Simulation.cpp \
                        $(SRC_DIR)/utils/Logger.cpp \
                        $(SRC_DIR)/schedulers/DynamicPriorityScheduler.cpp \
                        $(SRC_DIR)/schedulers/NonPreemptiveScheduler.cpp \
                        $(SRC_DIR)/schedulers/PreemptiveScheduler.cpp \
                        $(SRC_DIR)/schedulers/StaticPriorityScheduler.cpp \
                        $(SRC_DIR)/models/Process.cpp

# Output Binaries
TARGET = $(OUTPUT_DIR)/scheduler_simulation
TEST_TARGETS = $(OUTPUT_DIR)/test_process $(OUTPUT_DIR)/test_scheduler $(OUTPUT_DIR)/test_simulation

# Default Target (Build Everything)
all: $(TARGET) test

# Build the main scheduler simulation
$(TARGET): $(SRC_FILES)
	mkdir -p $(OUTPUT_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

# # Ensure config.json is copied to output
# $(OUTPUT_DIR)/config.json: $(SRC_DIR)/simulation/config.json
# 	cp $< $@

# Ensure logs.txt can be created
$(OUTPUT_DIR)/logs.txt:
	touch $@

# Build and run tests
test: $(TEST_TARGETS)
	@echo "Running Tests..."
	@for test in $(TEST_TARGETS); do $$test; done

# Test Executables
$(OUTPUT_DIR)/test_process: $(TEST_PROCESS_FILES)
	$(CLANGXX) $(CXXFLAGS) $(GTEST_INCLUDES) $^ -o $@ $(GTEST_FLAGS)

$(OUTPUT_DIR)/test_scheduler: $(TEST_SCHEDULER_FILES)
	$(CLANGXX) $(CXXFLAGS) $(GTEST_INCLUDES) $^ -o $@ $(GTEST_FLAGS)

$(OUTPUT_DIR)/test_simulation: $(TEST_SIMULATION_FILES)
	$(CLANGXX) $(CXXFLAGS) $(GTEST_INCLUDES) $(JSON_INCLUDES) $^ -o $@ $(GTEST_FLAGS)

# Clean compiled files
clean:
	rm -rf $(OUTPUT_DIR)/*.o $(OUTPUT_DIR)/*.out $(OUTPUT_DIR)/*

# Run the simulation (ensures required files exist)
run: $(TARGET) $(SIMULATION_DIR)/config.json $(OUTPUT_DIR)/logs.txt
	./$(TARGET)

# Help command
help:
	@echo "Makefile for Priority-Based Scheduling"
	@echo "Usage:"
	@echo "  make            - Build the project"
	@echo "  make run        - Run the simulation"
	@echo "  make test       - Compile and run tests"
	@echo "  make clean      - Remove compiled files"