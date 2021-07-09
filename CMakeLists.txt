cmake_minimum_required(VERSION 3.16)
project(RISC_V_Simulator)

set(CMAKE_CXX_STANDARD 14)

add_definitions(-O3)

add_executable(code
        CMakeLists.txt
        main.cpp
        cpu_stages.cpp
        components.hpp
        cpu_core.hpp
        decoder.hpp
        include.hpp
        )
