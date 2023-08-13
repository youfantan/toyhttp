#include "Server.h"
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include "./Utils.h"

template<int WorkerSize>
Server<WorkerSize>::Server(const std::string &bindAddress, uint16_t bindPort, std::array<Worker, WorkerSize> &workers) : workers(workers){
    //Create Socket and then listen
    sockFd = socket(AF_INET, SOCK_STREAM, 0);
    auto [addr, addrLen] = StringToSocketAddress(bindAddress, bindPort);
    if (addr == nullptr) {
        ERROR("Error at creating server: Invalid address");
        exit(EXIT_FAILURE);
    }
    ERRIFN1(bind(sockFd, addr, addrLen), "Error at creating server")
    ERRIFN1(::listen(sockFd, SOMAXCONN), "Error at creating server")
    delete[] addr;
}

template<int WorkerSize>
Server<WorkerSize>::~Server() {
    close(sockFd);
}

template<int WorkerSize>
void Server<WorkerSize>::listen() {
    int epollFd = epoll_create(1);
    EpollAddFd(epollFd, sockFd, EPOLLIN);
    epoll_event events[MaxEpollEvents] = {0};
    while (!stopFlag) {
        int evtNum = epoll_wait(epollFd, events, MaxEpollEvents, 1);
        for (int i = 0; i < evtNum; ++i) {
            if (events[i].data.fd == sockFd) {
                TRACE("New Client Connected");
                sockaddr *sockAddr;
                socklen_t sockAddrLen = 0;
                int cliFd;
                if ((cliFd = accept(sockFd, sockAddr, &sockAddrLen)) != -1) {
                    EpollAddFd(epollFd, cliFd, EPOLLIN);
                } else {
                    ERRORE("Error at accepting socket");
                }
            } else {
                if (events[i].data.fd != -1) {
                    if (workerNum == WorkerSize) {
                        workerNum = 0;
                    }
                    workers[workerNum].AssignFd(events[i].data.fd);
                    epoll_ctl(epollFd, EPOLL_CTL_DEL, events[i].data.fd, &events[i]);
                    close(events[i].data.fd);
                }
            }
        }
    }
}

template<int WorkerSize>
void Server<WorkerSize>::stop() {
    stopFlag = true;
}