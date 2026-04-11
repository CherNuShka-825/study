cmake_minimum_required(VERSION 4.1)
project(lab1)

set(CMAKE_CXX_STANDARD 20)

find_package(MPI REQUIRED)

add_executable(lab1 main.cpp
        type-1.cpp
        type-2.cpp
        src/timed-type-1.cpp
)

target_link_libraries(lab1 MPI::MPI_CXX)