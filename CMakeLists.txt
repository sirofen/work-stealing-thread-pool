cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-pthread -latomic")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Debug)

project(threads)

file(GLOB_RECURSE SOURCES "${PROJECT_SOURCE_DIR}/src/*.cpp")

find_package(benchmark REQUIRED)

message(sources)
include_directories(include)
# Valgrind
#find_program( MEMORYCHECK_COMMAND valgrind )#/
#set(MEMORYCHECK_COMMAND_OPTIONS "--trace-children=yes --leak-check=full")

include(CTest)
include(GoogleTest)

add_executable(threads main.cpp ${SOURCES})

target_link_libraries(threads benchmark::benchmark)

#add_test(threads_test threads)
add_test(threads_test threads)

#gtest_discover_tests(threads)
