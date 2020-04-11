cmake_minimum_required(VERSION 3.2)
project(rocket CXX)

option(BUILD_EXAMPLES "Build examples" ON)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# generate compile_commands.json if possible
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# include dir of rocket.hpp
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

if(BUILD_EXAMPLES)
    message(STATUS "build examples")
    add_executable(example example.cpp)    
endif()
