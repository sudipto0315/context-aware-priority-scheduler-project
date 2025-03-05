#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <fstream>
#include <string>
#include <mutex>
#include <sstream>

class Logger {
private:
    static std::ofstream logFile; // Log file stream
    static bool consoleOutput;    // Enable/disable console logging
    static std::mutex logMutex;   // Mutex for thread-safe logging

public:
    static void init(const std::string& filename, bool enableConsoleOutput = true);
    static void log(const std::string& message);
    static void logError(const std::string& errorMessage);
    static void logWarning(const std::string& warningMessage);
    static void log(const std::ostringstream& stream);
    static void setLogFile(const std::string& newFilename);
    static void close();
};

#endif // LOGGER_H