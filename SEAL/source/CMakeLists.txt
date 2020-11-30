cmake_minimum_required (VERSION 3.9...3.17)

project(eval_benchmark)

find_package(SEAL 3.5 CONFIG REQUIRED)

set(CMAKE_BUILD_TYPE RELEASE)

# target_link_libraries(main PRIVATE SEAL::seal MSGSL::MSGSL)

# Cardio BFV with CinguParam parameters
add_executable(cardio_bfv cardio-bfv/cardio.cpp common.h)
set_target_properties(cardio_bfv PROPERTIES LINKER_LANGUAGE CXX) 
target_link_libraries(cardio_bfv SEAL::seal)

# Cardio BFV with optimal parameters
add_executable(cardio_bfv_opt cardio-bfv-OPT-PARAMS/cardio.cpp common.h)
set_target_properties(cardio_bfv_opt PROPERTIES LINKER_LANGUAGE CXX) 
target_link_libraries(cardio_bfv_opt SEAL::seal)

# Cardio BFV "naive"
add_executable(cardio_bfv_naive cardio-bfv-naive/cardio.cpp common.h)
set_target_properties(cardio_bfv_naive PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(cardio_bfv_naive SEAL::seal)

# Cardio batched BFV
add_executable(cardio_bfv_batched cardio-bfv-batched/cardio-batched.cpp common.h)
set_target_properties(cardio_bfv_batched PROPERTIES LINKER_LANGUAGE CXX) 
target_link_libraries(cardio_bfv_batched SEAL::seal)

# Cardio batched CKKS
add_executable(cardio_ckks_batched cardio-ckks-batched/cardio-batched.cpp common.h)
set_target_properties(cardio_ckks_batched PROPERTIES LINKER_LANGUAGE CXX) 
target_link_libraries(cardio_ckks_batched SEAL::seal)

# NN batched CKKS
add_library(nn_ckks_batched_lib)
target_sources(nn_ckks_batched_lib PUBLIC
        common.h
        nn-ckks-batched/nn-batched.cpp
        nn-ckks-batched/helpers.h
        nn-ckks-batched/matrix_vector.cpp
        nn-ckks-batched/matrix_vector_crypto.cpp
        )
set_target_properties(nn_ckks_batched_lib PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(nn_ckks_batched_lib SEAL::seal)
add_executable(nn_ckks_batched)
set_target_properties(nn_ckks_batched PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(nn_ckks_batched nn_ckks_batched_lib SEAL::seal)

# NN batched CKKS tests
# add_subdirectory(nn-ckks-batched/tests)

# Chi Squared BFV
add_executable(chi_squared chi-squared-bfv/chi_squared.cpp)
set_target_properties(chi_squared PROPERTIES LINKER_LANGUAGE CXX) 
target_link_libraries(chi_squared SEAL::seal)

# Chi Squared BFV Batched
add_executable(chi_squared_batched chi-squared-bfv-batched/chi_squared_batched.cpp)
set_target_properties(chi_squared_batched PROPERTIES LINKER_LANGUAGE CXX) 
target_link_libraries(chi_squared_batched SEAL::seal)

#  Kernel BFV
add_executable(kernel kernel-bfv/kernel.cpp)
set_target_properties(kernel PROPERTIES LINKER_LANGUAGE CXX) 
target_link_libraries(kernel SEAL::seal)

#  Kernel BFV batched
add_executable(kernel_batched kernel-bfv-batched/kernel_batched.cpp)
set_target_properties(kernel_batched PROPERTIES LINKER_LANGUAGE CXX) 
target_link_libraries(kernel_batched SEAL::seal)
