cmake_minimum_required(VERSION 3.29)
project(lab1_a)

set(CMAKE_CXX_STANDARD 20)

add_executable(lab1_a
        main.cpp
        BitArray.cpp
)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
FetchContent_MakeAvailable(googletest)

add_executable(TestBitArray
        TestBItArray.cpp
        BitArray.cpp
)
target_include_directories(TestBitArray PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(TestBitArray PRIVATE GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(TestBitArray)
