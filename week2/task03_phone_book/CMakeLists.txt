cmake_minimum_required(VERSION 3.17)
project(task03_phone_book)


find_package(Protobuf REQUIRED) # Команда находит пути к компилятору protoc и библиотеке libprotobuf
include_directories(${Protobuf_INCLUDE_DIRS}) # Добавляем в include path пути к библиотеке protobuf
include_directories(${CMAKE_CURRENT_BINARY_DIR}) # В ${CMAKE_CURRENT_BINARY_DIR} будут файлы, сгенерированные компилятором protoс, путь к ним надо добавить в include path

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS contact.proto) # Здесь надо перечислить все ваши .proto-файлы

find_path(TEST_RUNNER_PATH test_runner.h)
include_directories(${TEST_RUNNER_PATH})

set(CMAKE_CXX_STANDARD 17)

add_executable(task03_phone_book main.cpp phone_book.cpp phone_book.h ${TEST_RUNNER_PATH}/test_runner.h ${PROTO_SRCS} ${PROTO_HDRS} ) # Здесь надо перечислить все ваши .cpp-файлы, в том числе и сгенерированные protoc'ом
target_link_libraries(task03_phone_book ${Protobuf_LIBRARIES}) # компонуем наш исполняемый файл с библиотекой libprotobuf