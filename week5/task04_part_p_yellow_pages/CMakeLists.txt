cmake_minimum_required(VERSION 3.17)
project(task04_part_p_yellow_pages)

set(CMAKE_CXX_STANDARD 17)

find_package(Protobuf REQUIRED)

include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

protobuf_generate_cpp(
        PROTO_SRCS
        PROTO_HDRS
        proto/transport_catalog.proto
        proto/address.proto
        proto/company.proto
        proto/database.proto
        proto/name.proto
        proto/phone.proto
        proto/rubric.proto
        proto/sphere.proto
        proto/url.proto
        proto/working_time.proto
)


#find_path(TEST_RUNNER_PATH test_runner.h)
#include_directories(${TEST_RUNNER_PATH})

# Build, execution, deployment -> CMake
# -DCMAKE_PREFIX_PATH=C:\Users\Name\protobuf -DTEST_RUNNER_PATH=C:\Users\Name\CLionProjects\coursera_cpp_course5\week3\task02_lexer
# -DCMAKE_PREFIX_PATH=C:\Users\Name\protobuf -DTEST_RUNNER_PATH=C:\Users\freez\CLionProjects\coursera_cpp_course5\week3\task02_lexer
# -DCMAKE_PREFIX_PATH=C:\Users\Name\protobuf -DTEST_RUNNER_PATH=/home/name/CLionProjects/coursera_cpp_course5/week3/task02_lexer

#add_definitions(-D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g -fno-omit-frame-pointer")


add_executable(task04_part_p_yellow_pages ${PROTO_SRCS} ${PROTO_HDRS} descriptions.cpp json.cpp main.cpp map_renderer.cpp requests.cpp serialization.cpp
        sphere.cpp svg.cpp transport_catalog.cpp transport_router.cpp utils.cpp yellow_pages.cpp yellow_pages_search.cpp)

target_link_libraries(task04_part_p_yellow_pages ${Protobuf_LIBRARIES})