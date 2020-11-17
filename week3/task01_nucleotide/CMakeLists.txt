cmake_minimum_required(VERSION 3.17)
project(task01_nucleotide)

find_path(TEST_RUNNER_PATH test_runner.h)
include_directories(${TEST_RUNNER_PATH})

# -DCMAKE_PREFIX_PATH=C:\Users\Name\protobuf -DTEST_RUNNER_PATH=C:\Users\Name\CLionProjects\coursera_cpp_course5\week2\task02_de_serialize

set(CMAKE_CXX_STANDARD 17)

add_executable(task01_nucleotide nucleotide_test.cpp)
