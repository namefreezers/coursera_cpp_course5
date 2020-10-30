cmake_minimum_required(VERSION 3.17)
project(task01_bad_domains_sanitizer)

add_compile_options(-fsanitize=address -g3 -fno-omit-frame-pointer)

set(CMAKE_CXX_STANDARD 17)

add_executable(task01_bad_domains_sanitizer san_fail.cpp)