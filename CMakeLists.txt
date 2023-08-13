cmake_minimum_required(VERSION 3.26)
project(toyhttp)

set(CMAKE_CXX_STANDARD 23)

add_executable(toyhttp main.cpp
        src/Server.cpp
        src/Server.h
        src/Utils.h
        src/Utils.cpp
        src/Worker.cpp
        src/Worker.h
        src/Defines.h
)
