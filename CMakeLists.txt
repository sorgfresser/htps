cmake_minimum_required(VERSION 3.25)
project(htps)

set(CMAKE_CXX_STANDARD 23)

add_executable(htps src/main.cpp src/base.cpp src/graph.cpp)
