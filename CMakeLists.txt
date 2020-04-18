cmake_minimum_required(VERSION 3.15)
project(multiset2)

set(CMAKE_CXX_STANDARD 20)

include_directories(include)

add_subdirectory(submodules/gtest)
enable_testing()
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
add_subdirectory(tests)