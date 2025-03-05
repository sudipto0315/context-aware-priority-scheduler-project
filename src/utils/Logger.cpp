#include "Logger.h"
#include <mutex>

// Initialize static members
std::ofstream Logger::logFile;
bool Logger::consoleOutput = true;
std::mutex Logger::logMutex;

// Initializes the logger by opening a log file
void Logger::init(const std::string& filename, bool enableConsoleOutput) {
    std::lock_guard<std::mutex> lock(logMutex);
    
    if (logFile.is_open()) {
        logFile.close(); // Close existing file before opening a new one
    }

    logFile.open(filename, std::ios::out | std::ios::app);
    if (!logFile) {
        std::cerr << "[ERROR] Could not open log file: " << filename << "\n";
    }
    
    consoleOutput = enableConsoleOutput;
}

// Logs a message to the console and file
void Logger::log(const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex);

    if (consoleOutput) {
        std::cout << message << std::endl;
    }
    if (logFile) {
        logFile << message << std::endl;
    }
}

// Logs an error message
void Logger::logError(const std::string& errorMessage) {
    log("[ERROR] " + errorMessage);
}

// Logs a warning message
void Logger::logWarning(const std::string& warningMessage) {
    log("[WARNING] " + warningMessage);
}

// Logs a formatted string stream message
void Logger::log(const std::ostringstream& stream) {
    log(stream.str());
}

// Changes the log file dynamically
void Logger::setLogFile(const std::string& newFilename) {
    std::lock_guard<std::mutex> lock(logMutex);

    if (logFile.is_open()) {
        logFile.close();
    }

    logFile.open(newFilename, std::ios::out | std::ios::app);
    if (!logFile) {
        std::cerr << "[ERROR] Could not open new log file: " << newFilename << "\n";
    }
}

// Closes the log file
void Logger::close() {
    std::lock_guard<std::mutex> lock(logMutex);

    if (logFile) {
        logFile.close();
    }
}