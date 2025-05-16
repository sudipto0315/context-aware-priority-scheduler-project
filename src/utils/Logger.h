#ifndef LOGGER_H
#define LOGGER_H

#include <fstream>
#include <streambuf>
#include <iostream>

class Logger {
public:
    static void init(const std::string& filename);
    static void close();

private:
    class TeeBuf : public std::streambuf {
    public:
        TeeBuf(std::streambuf* sb1, std::streambuf* sb2);
    protected:
        virtual int overflow(int c) override;
        virtual int sync() override;
    private:
        std::streambuf* m_sb1;
        std::streambuf* m_sb2;
    };

    static std::ofstream logFile;
    static TeeBuf* teeBuf;
    static std::streambuf* oldCoutBuf;
};

#endif // LOGGER_H
