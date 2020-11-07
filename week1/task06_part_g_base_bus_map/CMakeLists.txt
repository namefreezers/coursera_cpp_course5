cmake_minimum_required(VERSION 3.17)
project(task06_part_g_base_bus_map)

set(CMAKE_CXX_STANDARD 17)

add_executable(task06_part_g_base_bus_map descriptions.cpp descriptions.h graph.h json.cpp json.h main.cpp requests.cpp requests.h router.h
        sphere.cpp sphere.h svg.cpp transport_catalog.cpp transport_catalog.h transport_router.cpp transport_router.h utils.cpp utils.h)