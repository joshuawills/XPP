SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
cmake_minimum_required(VERSION 3.10)
project(compiler)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror")

add_executable(compiler src/compiler.cpp src/lexer.cpp src/token.cpp)


target_sources(compiler PRIVATE src/lexer.hpp)
target_sources(compiler PRIVATE src/token.hpp)
