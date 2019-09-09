cmake_minimum_required(VERSION 3.1)

project(msgpack-cpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Winline")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

include_directories(.)

add_executable(tests tests/test.cpp)

add_executable(iotest tests/iotest.cpp)

include_directories(perfomance/msgpack-c/include)
add_executable(pc perfomance/perf_comp.cpp)
