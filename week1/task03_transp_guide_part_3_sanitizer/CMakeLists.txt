cmake_minimum_required(VERSION 3.17)
project(task03_transp_guide_part_3_sanitizer)

set(CMAKE_CXX_STANDARD 17)

add_definitions(-D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g -fno-omit-frame-pointer")

add_executable(task03_transp_guide_part_3_sanitizer descriptions.cpp descriptions.h graph.h json.cpp json.h main.cpp
        requests.cpp requests.h router.h sphere.cpp sphere.h transport_catalog.cpp transport_catalog.h transport_router.cpp transport_router.h utils.cpp utils.h)