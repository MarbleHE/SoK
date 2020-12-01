cmake_minimum_required (VERSION 3.9...3.17)

project(eval_benchmark)

find_package(SEAL 3.5 CONFIG REQUIRED)

set(CMAKE_BUILD_TYPE RELEASE)

# target_link_libraries(main PRIVATE SEAL::seal MSGSL::MSGSL)

# Cardio BFV "OPT" with manually selected params
add_executable(cardio_bfv_manualparams cardio-bfv-opt/cardio.cpp common.h)
target_compile_definitions(cardio_bfv_manualparams PRIVATE MANUALPARAMS)
set_target_properties(cardio_bfv_manualparams PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(cardio_bfv_manualparams SEAL::seal)

# Cardio BFV "OPT" with CinguParam parameters
add_executable(cardio_bfv_cinguparam cardio-bfv-opt/cardio.cpp common.h)
target_compile_definitions(cardio_bfv_cinguparam PRIVATE CINGUPARAM)
set_target_properties(cardio_bfv_cinguparam PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(cardio_bfv_cinguparam SEAL::seal)

# Cardio BFV "OPT" with moduli selected by SEAL
add_executable(cardio_bfv_sealparams cardio-bfv-opt/cardio.cpp common.h)
target_compile_definitions(cardio_bfv_sealparams PRIVATE SEALPARAMS)
set_target_properties(cardio_bfv_sealparams PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(cardio_bfv_sealparams SEAL::seal)


# Cardio BFV "naive" with same manually selected params as manualparams
add_executable(cardio_bfv_naive_manualparams cardio-bfv-naive/cardio.cpp common.h)
target_compile_definitions(cardio_bfv_naive_manualparams PRIVATE MANUALPARAMS)
set_target_properties(cardio_bfv_naive_manualparams PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(cardio_bfv_naive_manualparams SEAL::seal)

# Cardio BFV "naive" with cinguparam parameters
add_executable(cardio_bfv_naive_cinguparam cardio-bfv-naive/cardio.cpp common.h)
target_compile_definitions(cardio_bfv_naive_cinguparam PRIVATE CINGUPARAM)
set_target_properties(cardio_bfv_naive_cinguparam PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(cardio_bfv_naive_cinguparam SEAL::seal)

# Cardio BFV "naive" with default seal parameters
add_executable(cardio_bfv_naive_sealparams cardio-bfv-naive/cardio.cpp common.h)
target_compile_definitions(cardio_bfv_naive_sealparams PRIVATE SEALPARAMS)
set_target_properties(cardio_bfv_naive_sealparams PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(cardio_bfv_naive_sealparams SEAL::seal)


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
