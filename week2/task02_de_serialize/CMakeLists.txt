cmake_minimum_required(VERSION 3.17)
project(task02_de_serialize)

add_definitions(-D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g -fno-omit-frame-pointer")

set(CMAKE_CXX_STANDARD 17)

add_executable(task02_de_serialize saveload.h test.cpp test_runner.h)