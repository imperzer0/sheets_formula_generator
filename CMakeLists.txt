cmake_minimum_required(VERSION 4.0)
project(sheets_formula_generator)

set(CMAKE_CXX_STANDARD 26)

add_executable(sheets_formula_generator main.cpp
        run.h
        run.cpp)
