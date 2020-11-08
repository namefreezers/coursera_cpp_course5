cmake_minimum_required(VERSION 3.17)
project(task08_part_i_layers_order)

set(CMAKE_CXX_STANDARD 17)

add_executable(task08_part_i_layers_order descriptions.cpp descriptions.h graph.h json.cpp json.h main.cpp requests.cpp requests.h router.h
        sphere.cpp sphere.h svg.cpp svg.h transport_catalog.cpp transport_catalog.h transport_router.cpp transport_router.h utils.cpp utils.h map_renderer.cpp map_renderer.h)