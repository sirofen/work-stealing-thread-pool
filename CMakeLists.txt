cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-Wall -Wextra -pedantic -O3")
set(CMAKE_CXX_STANDARD 20)

project(tp_tests)

find_package(TBB REQUIRED)
find_package(GTest REQUIRED)

enable_testing()

add_executable(tp_tests tests.cpp)
target_include_directories(tp_tests PUBLIC include)
target_link_libraries(tp_tests GTest::gtest_main TBB::tbb)

gtest_discover_tests(tp_tests)
