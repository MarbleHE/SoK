cmake_minimum_required (VERSION 3.9...3.17)

project(eval_benchmark)

find_package(SEAL 3.5 REQUIRED)

set(CMAKE_BUILD_TYPE DEBUG)

# Kernel (Laplacian Sharpening Algorithm)
# add_executable(laplacian laplacian_sharpening/laplacian_sharpening_batched.cpp)
# set_target_properties(laplacian PROPERTIES LINKER_LANGUAGE CXX) 
# target_link_libraries(laplacian SEAL::seal)

# Cardio
add_executable(cardio cardio/cardio.cpp)
set_target_properties(cardio PROPERTIES LINKER_LANGUAGE CXX) 
target_link_libraries(cardio SEAL::seal)
