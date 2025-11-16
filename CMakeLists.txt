cmake_minimum_required(VERSION 3.13)
project(bounceping)
set(CMAKE_CXX_STANDARD 26)

add_executable(bounceping src/main.cpp
        include/settings.hpp
        src/settings.cpp
        include/utils.hpp
        src/utils.cpp
        include/server.hpp
        src/server.cpp
        include/client.hpp
        src/client.cpp)

include_directories(include)