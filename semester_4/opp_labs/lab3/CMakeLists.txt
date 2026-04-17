cmake_minimum_required(VERSION 3.28)
project(lab3 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(MPI REQUIRED)

add_executable(lab3
        main.cpp
        test.cpp
)

target_link_libraries(lab3 PRIVATE MPI::MPI_CXX)