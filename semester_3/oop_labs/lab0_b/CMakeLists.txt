cmake_minimum_required(VERSION 3.29)
project(lab0_b)

set(CMAKE_CXX_STANDARD 20)

add_library(csv_lib
        CSV.cpp
)
target_include_directories(csv_lib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(
        lab0_b
        main.cpp
)
target_link_libraries(lab0_b PRIVATE csv_lib)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(csv_tests
        tests.cpp
)
target_link_libraries(csv_tests
        PRIVATE
        csv_lib
        GTest::gtest
        GTest::gtest_main
)

target_compile_definitions(csv_tests PRIVATE CSV_ENABLE_TESTS)

include(GoogleTest)
add_test(NAME csv_tests COMMAND csv_tests)