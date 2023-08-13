#include <string>
#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <unordered_map>
#include "Worker.h"
#include "Utils.h"

inline std::tuple<int, int> RecvMsgHdr(int sockFd, sockaddr *serverSideAddress, socklen_t serverSideAddressLen) {
    char ivBuffer = 0;
    iovec iv = {&ivBuffer, 1};
    char cmsgFd[CMSG_LEN(sizeof(int))];
    msghdr m{nullptr, 0, &iv, 1, cmsgFd, sizeof(cmsgFd), 0};
    ERRIFN1(recvmsg(sockFd, &m, 0), "Error at receiving fd from Reactor")
    char receipt = 0x41;
    iovec receiptIv = {&receipt, 1};
    msghdr receiptMsg{serverSideAddress, serverSideAddressLen, &receiptIv, 1, nullptr, 0, 0};
    ERRIFN1(sendmsg(sockFd, &receiptMsg, 0), "Error at sending single receipt")
    if (ivBuffer == 0x00) {
        cmsghdr *c = CMSG_FIRSTHDR(&m);
        int fd = *reinterpret_cast<int *>(CMSG_DATA(c));
        return std::make_tuple(ivBuffer, fd);
    } else {
        return std::make_tuple(ivBuffer, -1);
    }
}

inline bool SendMsgHdr(int sockFd, sockaddr *to, socklen_t toLen, char *externalData, int externalDataLen, int externalFileDescriptor) {
    iovec iv{};
    iv.iov_base = externalData;
    iv.iov_len = externalDataLen;
    msghdr hdr{};
    if (externalFileDescriptor != -1) {
        char cmsg[CMSG_SPACE(sizeof(int))];
        hdr = {to, toLen, &iv, 1, cmsg, sizeof(cmsg), 0};
        cmsghdr *c = CMSG_FIRSTHDR(&hdr);
        if (c == nullptr) {
            ERROR("Error at assigning new task: struct CMSGHDR is null");
            return false;
        }
        c->cmsg_level = SOL_SOCKET;
        c->cmsg_type = SCM_RIGHTS;
        c->cmsg_len = CMSG_LEN(sizeof(int));
        *reinterpret_cast<int *>(CMSG_DATA(c)) = externalFileDescriptor;
    } else {
        hdr = {to, toLen, &iv, 1, nullptr, 0, 0};
    }
    if (sendmsg(sockFd, &hdr, 0) == -1) {
        ERRORE("Error at sending message");
        return false;
    }
    char receipt = 0;
    iovec recvMsgIv {&receipt, 1};
    msghdr recvMsg {nullptr, 0, &recvMsgIv, 1, nullptr, 0, 0};
    if (recvmsg(sockFd, &recvMsg, 0) == -1) {
        ERRORE("Error at receiving single receipt");
    }
    if (receipt == 0x41) {
        return true;
    }
    return false;
}

Worker::Worker(int instanceNum, int serialNum) {
    std::string unServerPath = std::format("/tmp/toyhttp_socket_{}_{}_server", instanceNum, serialNum);
    std::string unClientPath = std::format("/tmp/toyhttp_socket_{}_{}_client", instanceNum, serialNum);
    bzero(&serverSideAddress, sizeof(serverSideAddress));
    bzero(&clientSideAddress, sizeof(clientSideAddress));
    serverSideAddress.sun_family = AF_UNIX;
    clientSideAddress.sun_family = AF_UNIX;
    memcpy(serverSideAddress.sun_path, unServerPath.c_str(), unServerPath.size());
    memcpy(clientSideAddress.sun_path, unClientPath.c_str(), unClientPath.size());
    int unServerFd = socket(AF_UNIX, SOCK_DGRAM, 0);
    int unClientFd = socket(AF_UNIX, SOCK_DGRAM, 0);
    serverSideFd = unServerFd;
    clientSideFd = unClientFd;
    if (bind(unServerFd, reinterpret_cast<const sockaddr *>(&serverSideAddress), sizeof(serverSideAddress)) == -1) {
        ERRORE("Error at bind server-side unix socket");
        exit(EXIT_FAILURE);
    }
    if (bind(unClientFd, reinterpret_cast<const sockaddr *>(&clientSideAddress), sizeof(clientSideAddress)) == -1) {
        ERRORE("Error at bind client-side unix socket");
        exit(EXIT_FAILURE);
    }
    int pid = fork();
    if (pid == 0) {
        close(unServerFd);
        //listen
        int epollFd = epoll_create(1);
        EpollAddFd(epollFd, unClientFd, EPOLLIN);
        epoll_event events[MaxEpollEvents] = {0};
        std::unordered_map<int, buffer> buffers;
        while (true) {
            int evtNum = epoll_wait(epollFd, events, MaxEpollEvents, -1);
            for (int i = 0; i < evtNum; ++i) {
                if (events[i].data.fd == unClientFd) {
                    auto [ret, fd] = RecvMsgHdr(unClientFd, reinterpret_cast<sockaddr *>(&serverSideAddress), sizeof(serverSideAddress));
                    if (ret == 0x00) {
                        EpollAddFd(epollFd, fd, EPOLLIN);
                    } else {
                        close(clientSideFd);
                        break;
                    }
                } else {
                    int fd = events[i].data.fd;
                    if (!buffers.contains(fd)) {
                        buffers[fd] = buffer();
                    }
                    buffer buffer = buffers[fd];
                    char kBuffer[32767] = {0};
                    size_t bytesRead = read(fd, kBuffer, 32767);
                    if (bytesRead == 0) {
                        buffer.write(kBuffer, bytesRead);
                        buffers.erase(fd);
                        //send buffer to handler
                        std::cout << buffer.get() << std::endl;
                        buffer.free();
                        epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, &events[i]);
                        close(fd);
                    } else if (bytesRead == -1){
                        ERRORE("Error at reading buffer");
                        buffer.free();
                        buffers.erase(fd);
                        epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, &events[i]);
                        close(fd);
                    } else {
                        buffer.write(kBuffer, bytesRead);
                    }
                }
            }
        }
    } else {
        close(unClientFd);
        workerPid = pid;
    }
}

int Worker::GetServerSideFd() const {
    return serverSideFd;
}

int Worker::GetClientSideFd() const {
    return clientSideFd;
}

int Worker::GetWorkerPid() const {
    return workerPid;
}

bool Worker::AssignFd(int fd) {
    char sign = 0x00;
    return SendMsgHdr(serverSideFd, reinterpret_cast<sockaddr *>(&clientSideAddress), sizeof(clientSideAddress), &sign, 1, fd);
}

Worker::~Worker() {
    char sign = 0x79;
    SendMsgHdr(serverSideFd, reinterpret_cast<sockaddr *>(&clientSideAddress), sizeof(clientSideAddress), &sign, 1, -1);
    close(serverSideFd);
}
