cmake_minimum_required(VERSION 3.17)
project(task03_mython)

find_path(TEST_RUNNER_PATH test_runner.h)
include_directories(${TEST_RUNNER_PATH})

# Build, execution, deployment -> CMake
# -DCMAKE_PREFIX_PATH=C:\Users\Name\protobuf -DTEST_RUNNER_PATH=C:\Users\Name\CLionProjects\coursera_cpp_course5\week3\task02_lexer
# -DCMAKE_PREFIX_PATH=C:\Users\Name\protobuf -DTEST_RUNNER_PATH=/home/name/CLionProjects/coursera_cpp_course5/week3/task02_lexer


#add_definitions(-D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g -fno-omit-frame-pointer")

set(CMAKE_CXX_STANDARD 17)

add_executable(task03_mython comparators.cpp lexer.cpp lexer_test.cpp mython.cpp object.cpp
        object_holder.cpp object_holder_test.cpp object_test.cpp parse.cpp parse_test.cpp statement.cpp statement_test.cpp)