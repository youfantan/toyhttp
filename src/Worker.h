#pragma once

#include "Defines.h"
#include <cstring>
#include <sys/un.h>

class Worker {
private:
    int serverSideFd;
    int clientSideFd;
    int workerPid;
    sockaddr_un serverSideAddress {};
    sockaddr_un clientSideAddress {};
public:
    Worker(int instanceNum, int serialNum);
    int GetServerSideFd() const;
    int GetClientSideFd() const;
    int GetWorkerPid() const;
    bool AssignFd(int fd);
    ~Worker();
};
