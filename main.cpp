#include <iostream>
#include <thread>
#include <cstring>
#include "./src/Utils.h"
#include "./src/Server.h"
#include "src/Worker.h"

#define SRC_ROOT_PATH "/home/gxglous/toyhttp/"
const char *SrcRootPath = SRC_ROOT_PATH;
const char *ProcessName = "Reactor";
size_t SrcRootPathLen = strlen(SrcRootPath);
int main() {
    LogInit();
    int n = RandomNum();
    std::array<Worker, 4> workers {Worker(n, 0), Worker(n, 1), Worker(n, 2), Worker(n, 3) };
    Server<4> server("127.0.0.1", 1145, workers);
    server.listen();
    LogStop();
}
