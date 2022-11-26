cmake_minimum_required(VERSION 3.21)
project(mython)

set(CMAKE_CXX_STANDARD 17)

add_executable(mython main.cpp
        lexer.cpp
        lexer.h
        lexer_test_open.cpp
        parse.cpp
        parse.h
        parse_test.cpp
        runtime.cpp
        runtime.h
        runtime_test.cpp
        statement.cpp
        statement.h
        statement_test.cpp)