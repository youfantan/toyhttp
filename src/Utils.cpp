#include "./Utils.h"
#include <sys/epoll.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <random>
#include <cstring>

extern const char *SrcRootPath;
extern size_t SrcRootPathLen;

const char *RelFileName(const char *FileName) {
    size_t FileNameLen = strlen(FileName);
    if (SrcRootPathLen == 0 && SrcRootPath != nullptr) {
        SrcRootPathLen = strlen(SrcRootPath);
    }
    if (SrcRootPathLen > FileNameLen) {
        return FileName;
    }
    size_t newFileNameLen = FileNameLen - SrcRootPathLen;
    char *newFileName = new char[newFileNameLen + 1];
    for (int i = 0; i < SrcRootPathLen; ++i) {
        if (SrcRootPath[i] != FileName[i]) {
            return FileName;
        }
    }
    memcpy(newFileName, FileName + SrcRootPathLen, newFileNameLen);
    newFileName[newFileNameLen] = 0;
    return newFileName;
}



template<LogLevel Level>
void LogOutWithErrno(const std::string &Msg, int Errno, const char *File, int Line) {
    char errnobuf[256] = {0};
    char *ret;
    if ((ret = strerror_r(Errno, errnobuf, 256)) != nullptr) {
        LogOut<Level>(std::format("{}({}): {}", ret, Errno, Msg), File, Line);
    } else {
        LogOut<Level>(std::format("Unknown Errno: {}", Msg), File, Line);
    }
}

template<LogLevel Level>
void LogOutWithErrno(const std::string &Msg, const char *File, int Line) {
    LogOutWithErrno<Level>(Msg, errno, File, Line);
}

std::tuple<sockaddr *, socklen_t> StringToSocketAddress(const std::string &Address, uint16_t Port) {
    //Count "dot"
    int dotCount = 0;
    for (auto &c : Address) {
        if (c == '.')
            dotCount++;
    }
    if (dotCount == 3) {
        //May be IPv4
        auto *output = static_cast<sockaddr_in *>(malloc(sizeof(sockaddr_in)));
        bzero(output, sizeof(sockaddr_in));
        output->sin_family = AF_INET;
        if (inet_pton(AF_INET, Address.c_str(), &output->sin_addr) != -1) {
            reinterpret_cast<sockaddr_in *>(output)->sin_port = htons(Port);
            return {reinterpret_cast<sockaddr *>(output), sizeof(sockaddr_in)};
        } else {
            ERROR("Error at converting string into IPv4 address");
            return {nullptr, 0};
        }
    } else if (dotCount != 0) {
        //May be IPv6
        auto *output = reinterpret_cast<sockaddr_in6 *>(malloc(sizeof(sockaddr_in6)));
        bzero(output, sizeof(sockaddr_in6));
        output->sin6_family = AF_INET6;
        if (inet_pton(AF_INET6, Address.c_str(), &output->sin6_addr) != -1) {
            reinterpret_cast<sockaddr_in6 *>(output)->sin6_port = htons(Port);
            return {reinterpret_cast<sockaddr *>(output), sizeof(sockaddr_in6)};;
        } else {
            ERROR("Error at converting string into IPv6 address");
            return {nullptr, 0};
        }
    } else {
        //TODO: May be Domain, parse to IPv4 or IPv6
        return {nullptr, 0};
    }
}

const char *SocketAddressToString(sockaddr *Address) {
    if (reinterpret_cast<char *>(Address)[0] == AF_INET) {
        //Parse as IPV4
        char *host = new char[16];
        if (inet_ntop(AF_INET, Address, host, 16) != nullptr) {
            return host;
        } else {
            ERRORE("Error at parsing socket address");
            return nullptr;
        }
    } else if (reinterpret_cast<char *>(Address)[0] == AF_INET6) {
        //Parse as IPV6
        char *host = new char[48];
        if (inet_ntop(AF_INET6, Address, host, 48) != nullptr) {
            return host;
        } else {
            ERRORE("Error at parsing socket address");
            return nullptr;
        }
    } else {
        ERROR("Unsupport TCP/IP protocol");
        return nullptr;
    }
}

static std::random_device rDev;
static std::ranlux48 rEngine(rDev());

int RandomNum(int from, int to) {
    std::uniform_int_distribution<> distribution(from, to);
    return distribution(rEngine);
}

inline int SetNonBlocking(int fd) {
    int opt = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, opt | O_NONBLOCK);
    return opt;
}
void EpollAddFd(int epollFd, int fd, int events) {
    epoll_event evt{};
    evt.events = events;
    evt.data.fd = fd;
    epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &evt);
    SetNonBlocking(fd);
}