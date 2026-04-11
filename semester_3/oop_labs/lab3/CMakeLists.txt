cmake_minimum_required(VERSION 3.29)
project(lab3)
set(CMAKE_CXX_STANDARD 20)

add_executable(lab3
        src/main.cpp
        src/audio_stream.cpp
        src/config_parser.cpp
        src/converter_factory.h
        src/pipeline.cpp
        src/converter.h
        converters/mute.cpp
        converters/mix.cpp
        converters/gain.cpp
)
target_include_directories(lab3 PRIVATE src converters)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
FetchContent_MakeAvailable(googletest)

add_executable(Tests
        src/audio_stream.cpp
        src/config_parser.cpp
        src/converter_factory.h
        src/pipeline.cpp
        src/converter.h
        converters/mute.cpp
        converters/mix.cpp
        converters/gain.cpp
        tests/test_audio_stream.cpp
        tests/test_config_parser.cpp
        tests/test_converter_factory.cpp
        tests/test_mute.cpp
        tests/test_mix.cpp
        tests/test_gain.cpp
        tests/test_pipeline.cpp
)
target_include_directories(Tests PRIVATE src converters)
target_link_libraries(Tests PRIVATE gtest_main)

enable_testing()
add_test(NAME Tests COMMAND Tests)