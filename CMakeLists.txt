cmake_minimum_required(VERSION 3.16)
project(RISC_V_Simulator)

set(CMAKE_CXX_STANDARD 14)

include_directories(src)

add_definitions(-O3)

add_executable(code
        CMakeLists.txt
        src/main.cpp
        src/cpu_stages.cpp
        src/components.hpp
        src/cpu_core.hpp
        src/decoder.hpp
        src/include.hpp
        )
