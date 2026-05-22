cmake_minimum_required(VERSION 3.16)

project(lab5 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(MPI REQUIRED)

add_executable(lab5
        main.cpp
        src/TaskQueue.cpp
        src/TaskGenerator.cpp
        src/Statistics.cpp
        src/WorkBalancer.cpp
)

target_include_directories(lab5 PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(lab5 PRIVATE
        MPI::MPI_CXX
)