#pragma once

#include <string>
#include <fstream>
#include <iostream>
#include <format>
#include <arpa/inet.h>
#include <climits>
#include <cstring>
#include <chrono>
#include "Defines.h"

//If the expression result is 0, output the error message
#define ERRIF0(expr)

//If the expression result is 1, output the error message
#define ERRIF1(expr)

//If the expression result is not 0, output the error message
#define ERRIFN0(expr)

//If the expression result is -1, output the error message and errno message
#define ERRIFN1(expr, msg) if (expr == -1) LogOutWithErrno<LogLevel::ERROR>(msg, errno, RelFileName(__FILE__), __LINE__);

#define TRACE(Message) LogOut<LogLevel::TRACE>(Message, RelFileName(__FILE__), __LINE__)
#define INFO(Message) LogOut<LogLevel::INFO>(Message, RelFileName(__FILE__), __LINE__)
#define WARNING(Message) LogOut<LogLevel::WARNING>(Message, RelFileName(__FILE__), __LINE__)
#define ERROR(Message) LogOut<LogLevel::ERROR>(Message, RelFileName(__FILE__), __LINE__)

#define TRACEE(Message) LogOutWithErrno<LogLevel::TRACE>(Message, RelFileName(__FILE__), __LINE__)
#define INFOE(Message) LogOutWithErrno<LogLevel::INFO>(Message, RelFileName(__FILE__), __LINE__)
#define WANRINGE(Message) LogOutWithErrno<LogLevel::WARNING>(Message, RelFileName(__FILE__), __LINE__)
#define ERRORE(Message) LogOutWithErrno<LogLevel::ERROR>(Message, RelFileName(__FILE__), __LINE__)

static std::ofstream LogOutStream;

extern const char *ProcessName;
const char *RelFileName(const char *FileName);

using buffer = struct tag_buffer{
    char *buffer;
    size_t capacity;
    size_t position;
    tag_buffer() {
        buffer = new char[DefaultBufferInitialSize];
        bzero(buffer, DefaultBufferInitialSize);
        capacity = DefaultBufferInitialSize;
        position = 0;
    }
    inline void write(char *ptr, size_t size) {
        if (size + position > capacity) {
            free();
            buffer = new char [size + position];
            bzero(buffer, size + position);
        }
        memcpy(buffer + position, ptr, size);
        position += size;
    }

    inline char* &get() {
        return buffer;
    }

    inline size_t getCapacity() {
        return capacity;
    }

    inline void free() const {
        delete[] buffer;
    }
};

inline int LogInit() {
    LogOutStream.open("toyhttp.log", std::ios::out);
    if (!LogOutStream.good()) {
        std::cout << "\033[31m" << "No permission to access the file toyhttp.log" << "\033[0m" << std::endl;
        exit(EXIT_FAILURE);
    }
    return 0;
}

enum LogLevel {
    TRACE,
    INFO,
    WARNING,
    ERROR
};

template<LogLevel Level>
void LogOut(const std::string &Msg,const char *File, int Line) {
    auto tm = std::chrono::current_zone()->to_local(std::chrono::system_clock::now());
    if constexpr (Level == LogLevel::TRACE) {
        auto msg = std::format("[{}][TRACE][{}:{}][{:%Y-%m-%d %T}] {}", ProcessName, File, Line, std::chrono::floor<std::chrono::seconds>(tm), Msg);
        std::cout << "\033[34m" << msg << "\033[0m" << std::endl;
        LogOutStream << msg << std::endl;
        return;
    }
    if constexpr (Level == LogLevel::INFO) {
        auto msg = std::format("[{}][INFO][{}:{}][{:%Y-%m-%d %T}] {}", ProcessName, File, Line, std::chrono::floor<std::chrono::seconds>(tm), Msg);
        std::cout << msg << std::endl;
        LogOutStream << msg << std::endl;
        return;
    }
    if constexpr (Level == LogLevel::WARNING) {
        auto msg = std::format("[{}][WARNING][{}:{}][{:%Y-%m-%d %T}] {}", ProcessName, File, Line, std::chrono::floor<std::chrono::seconds>(tm), Msg);
        std::cout << "\033[33m" << msg << "\033[0m" << std::endl;
        LogOutStream << msg << std::endl;
        return;
    }
    if constexpr (Level == LogLevel::ERROR) {
        auto msg = std::format("[{}][ERROR][{}:{}][{:%Y-%m-%d %H:%M:%S}] {}", ProcessName, File, Line, std::chrono::floor<std::chrono::seconds>(tm),Msg);
        std::cout << "\033[31m" << msg << "\033[0m" << std::endl;
        LogOutStream << msg << std::endl;
        return;
    }
}

template<LogLevel Level>
void LogOutWithErrno(const std::string &Msg, int Errno, const char *File, int Line);

template<LogLevel Level>
void LogOutWithErrno(const std::string &Msg, const char *File, int Line);

inline void LogStop() {
    LogOutStream.close();
}

std::tuple<sockaddr *, socklen_t> StringToSocketAddress(const std::string &Address, uint16_t Port);

const char *SocketAddressToString(sockaddr *Address);

int RandomNum(int from, int to);
inline int RandomNum() {
    return RandomNum(0, INT_MAX);
}
void EpollAddFd(int epollFd, int fd, int events);