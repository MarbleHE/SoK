cmake_minimum_required(VERSION 3.10)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

project(eval_benchmark)

set(CMAKE_CXX_STANDARD 17)

find_package(Cingulata REQUIRED)
find_package(Threads REQUIRED)

add_executable(benchmark main.cpp)

target_include_directories(benchmark PRIVATE ${Cingulata_INCLUDE_DIR})
target_link_libraries(benchmark PRIVATE ${Cingulata_LIBRARY})
target_link_libraries(benchmark PRIVATE Threads::Threads)