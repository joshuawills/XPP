cmake_minimum_required(VERSION 3.10)
project(compiler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror")

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_executable(compiler
    src/compiler.cpp src/lexer.cpp src/token.cpp src/parser.cpp
    src/ast.cpp src/decl.cpp src/stmt.cpp src/module.cpp
    src/type.cpp src/expr.cpp src/handler.cpp src/visitor.cpp src/verifier.cpp src/emitter.cpp
)

target_sources(compiler PRIVATE
    src/lexer.hpp src/token.hpp src/parser.hpp src/ast.hpp
    src/decl.hpp src/stmt.hpp src/module.hpp src/type.hpp
    src/expr.hpp src/handler.hpp src/visitor.hpp src/verifier.hpp src/emitter.hpp
)

set(LLVM_COMPONENTS
    core
    orcjit
    native
    nativecodegen
    support
)

llvm_map_components_to_libnames(LLVM_LIBS ${LLVM_COMPONENTS})
target_link_libraries(compiler PRIVATE ${LLVM_LIBS})

target_include_directories(compiler SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})