cmake_minimum_required(VERSION 3.10)
project(NQ_Backtester)

set(CMAKE_CXX_STANDARD 17)

# This is the magic line that lets you just say #include "parser.h"
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Tell CMake exactly where the files are
add_executable(NQ_Backtester 
    main.cpp 
    src/parser.cpp
)