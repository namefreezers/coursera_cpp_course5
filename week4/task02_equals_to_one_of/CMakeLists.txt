cmake_minimum_required(VERSION 3.17)
project(task02_equals_to_one_of)

find_path(TEST_RUNNER_PATH test_runner.h)
include_directories(${TEST_RUNNER_PATH})

# Build, execution, deployment -> CMake
# -DCMAKE_PREFIX_PATH=C:\Users\Name\protobuf -DTEST_RUNNER_PATH=C:\Users\Name\CLionProjects\coursera_cpp_course5\week3\task02_lexer
# -DCMAKE_PREFIX_PATH=C:\Users\Name\protobuf -DTEST_RUNNER_PATH=/home/name/CLionProjects/coursera_cpp_course5/week3/task02_lexer

set(CMAKE_CXX_STANDARD 17)

add_executable(task02_equals_to_one_of equals_to_one_of.cpp)