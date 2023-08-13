#pragma once

#include <string>
#include <cstdint>
#include "Worker.h"

template<int WorkerSize>
class Server {
private:
    int sockFd;
    bool stopFlag {false};
    int workerNum;
    std::array<Worker, WorkerSize>& workers;
public:
    Server() = delete;
    Server(Server &) = delete;
    Server(Server &&) = delete;

    Server(const std::string &bindAddress, uint16_t bindPort, std::array<Worker, WorkerSize>& workers);
    void listen();
    void stop();
    ~Server();
};
