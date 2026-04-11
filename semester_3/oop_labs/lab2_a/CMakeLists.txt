cmake_minimum_required(VERSION 3.29)
project(lab2_a)

set(CMAKE_CXX_STANDARD 20)

add_executable(lab2_a
        src/main.cpp
        src/GameController.cpp
        src/FileWork.cpp
        src/Universe.cpp
        src/Rule.cpp)
target_include_directories(lab2_a PRIVATE src)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
FetchContent_MakeAvailable(googletest)

add_executable(Tests
        tests/TestRule.cpp
        src/Rule.cpp
        tests/TestUniverse.cpp
        src/Universe.cpp
        tests/TestFileWork.cpp
        src/FileWork.cpp
        tests/TestGameController.cpp
        src/GameController.cpp
)
target_include_directories(Tests PRIVATE src)
target_link_libraries(Tests PRIVATE gtest_main)

enable_testing()
add_test(NAME Tests COMMAND Tests)
