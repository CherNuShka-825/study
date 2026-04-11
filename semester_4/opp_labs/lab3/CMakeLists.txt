cmake_minimum_required(VERSION 4.1)
project(lab3)

set(CMAKE_CXX_STANDARD 20)

find_package(MPI REQUIRED)

add_executable(lab3 main.cpp
        test.cpp)

target_link_libraries(lab3 MPI::MPI_CXX)
