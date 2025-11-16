cmake_minimum_required(VERSION 3.13)
project(bounceping)               # Create project "simple_example"
set(CMAKE_CXX_STANDARD 26)            # Enable c++14 standard

add_executable(bounceping src/main.cpp)

include_directories(include)