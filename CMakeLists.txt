cmake_minimum_required(VERSION 3.5)
#https://norasandler.com/2017/11/29/Write-a-Compiler.html

project(c-compiler-tutorial LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED)

#set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -m32 -O0 -ggdb")

source_group(EXAMPLES
    # first example
    examples/return_2.c
)

set(COMPILER_HDRS
    generator.h
    lexer.h
    parser.h
)

set(COMPILER_SRCS
    generator.cpp
    lexer.cpp
    main.cpp
    parser.cpp
)

add_executable(ccomp ${COMPILER_SRCS})
