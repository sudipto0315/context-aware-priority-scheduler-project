#include "Logger.h"

std::ofstream Logger::logFile;
Logger::TeeBuf* Logger::teeBuf = nullptr;
std::streambuf* Logger::oldCoutBuf = nullptr;

Logger::TeeBuf::TeeBuf(std::streambuf* sb1, std::streambuf* sb2)
    : m_sb1(sb1), m_sb2(sb2) {}

int Logger::TeeBuf::overflow(int c) {
    if (c == EOF) return !EOF;
    if (m_sb1) m_sb1->sputc(c);
    if (m_sb2) m_sb2->sputc(c);
    return c;
}

int Logger::TeeBuf::sync() {
    int r1 = m_sb1 ? m_sb1->pubsync() : 0;
    int r2 = m_sb2 ? m_sb2->pubsync() : 0;
    return r1 == 0 && r2 == 0 ? 0 : -1;
}

void Logger::init(const std::string& filename) {
    logFile.open(filename, std::ios::out | std::ios::app);
    if (!logFile.is_open()) {
        std::cerr << "[ERROR] Failed to open log file: " << filename << std::endl;
        return;
    }

    teeBuf = new TeeBuf(std::cout.rdbuf(), logFile.rdbuf());
    oldCoutBuf = std::cout.rdbuf(teeBuf); // redirect cout
}

void Logger::close() {
    if (teeBuf) {
        std::cout.rdbuf(oldCoutBuf); // restore original cout
        delete teeBuf;
        teeBuf = nullptr;
    }

    if (logFile.is_open()) {
        logFile.close();
    }
}
