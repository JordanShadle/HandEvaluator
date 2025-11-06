cmake_minimum_required(VERSION 3.16)
project(holdem_evaluator LANGUAGES CXX)

# Require C++20 and ensure it's enforced
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable good compiler warnings
if (MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O2)
endif()

# Add the executable
add_executable(holdem_evaluator holdem_evaluator.cpp)
