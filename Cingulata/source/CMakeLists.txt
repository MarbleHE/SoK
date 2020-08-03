cmake_minimum_required(VERSION 3.9)

project(cingulata VERSION 1.5.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

# compilation options
option(BUILD_DOC "Build documentation" OFF)
option(USE_BFV "Use B/FV implementation" ON)    # use BFV by default
set(TFHE_PATH "" CACHE PATH "TFHE library path")
option(ENABLE_UNITTEST OFF)
set(USE_TFHE OFF)
set(INIT_GOOGLETEST_MODULE OFF)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

if (USE_BFV)
  set(INIT_ABC_MODULE ON)
  set(INIT_CINGU_PARAM_MODULE ON)
  set(BUILD_ONLY_COMMON OFF)
else()
    message(FATAL_ERROR "Benchmarking environment only prepared for BFV. Aborting.")
endif (USE_BFV)

set(COMMON_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/common/include)

add_subdirectory(submodules)
add_subdirectory(common)

message("BUILD_DOC                  " ${BUILD_DOC})
message("BUILD_ONLY_COMMON          " ${BUILD_ONLY_COMMON})
message("USE_BFV                    " ${USE_BFV})
message("USE_TFHE                   " ${USE_TFHE})
message("TFHE_PATH                  " ${TFHE_PATH})
message("ENABLE_UNITTEST            " ${ENABLE_UNITTEST})
message("INIT_ABC_MODULE            " ${INIT_ABC_MODULE})
message("INIT_CINGU_PARAM_MODULE    " ${INIT_CINGU_PARAM_MODULE})
message("INIT_GOOGLETEST_MODULE     " ${INIT_GOOGLETEST_MODULE})

add_subdirectory(optim)
add_subdirectory(runtime)

include_directories(circuit/include) # support old circuit interface
include_directories(${COMMON_INCLUDE_DIR})

# directory of evaluation benchmarks
add_subdirectory(eval)
