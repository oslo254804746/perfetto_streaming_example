cmake_minimum_required(VERSION 3.22)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
include_directories(perfetto/sdk)
add_library(perfetto STATIC perfetto/sdk/perfetto.cc)
project(perfetto_streaming_example)
add_executable(perfetto_streaming_example main.cpp)
target_link_libraries(
        perfetto_streaming_example
        perfetto
        android
        log
)
