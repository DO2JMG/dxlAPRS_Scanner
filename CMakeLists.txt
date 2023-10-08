cmake_minimum_required(VERSION 3.5)

project (scanner)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

include_directories(include)

set(SOURCES
    src/main.cpp
    src/tools.cpp
    include/tools.h
)

add_executable(scanner ${SOURCES})
