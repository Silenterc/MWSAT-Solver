cmake_minimum_required(VERSION 4.0)
project(kop2)

set(CMAKE_CXX_STANDARD 17)

include_directories(.)

add_executable(kop2
        main.cpp
        solver.cpp
        solver.h)
