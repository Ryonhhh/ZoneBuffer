# Project properties
cmake_minimum_required(VERSION 3.8)
project(zbuftest)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

message("${CMAKE_BUILD_TYPE}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fPIC -pthread -O2 -fmax-errors=5")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -std=c++17 -O0 -g -Wall -static -DDEBUG")

include_directories(include)
add_subdirectory(src)

add_executable(main main.cpp)
target_link_libraries(main zbuf)
