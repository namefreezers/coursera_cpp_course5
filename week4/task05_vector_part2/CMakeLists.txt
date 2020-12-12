cmake_minimum_required(VERSION 3.17)
project(task05_vector_part2)

find_path(TEST_RUNNER_PATH test_runner.h)
include_directories(${TEST_RUNNER_PATH})

# Build, execution, deployment -> CMake
# -DCMAKE_PREFIX_PATH=C:\Users\Name\protobuf -DTEST_RUNNER_PATH=C:\Users\Name\CLionProjects\coursera_cpp_course5\week3\task02_lexer
# -DCMAKE_PREFIX_PATH=C:\Users\Name\protobuf -DTEST_RUNNER_PATH=/home/name/CLionProjects/coursera_cpp_course5/week3/task02_lexer

#add_definitions(-D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g -fno-omit-frame-pointer")

set(CMAKE_CXX_STANDARD 17)

add_executable(task05_vector_part2 test.cpp)