cmake_minimum_required (VERSION 3.9...3.17)

project(eval_benchmark)

find_package(SEAL 3.5 CONFIG REQUIRED)

set(CMAKE_BUILD_TYPE RELEASE)

# target_link_libraries(main PRIVATE SEAL::seal MSGSL::MSGSL)

# Microbenchmark BFV
add_executable(microbenchmark-bfv microbenchmark-bfv/microbenchmark.cpp common.h)
target_compile_definitions(microbenchmark-bfv PRIVATE SEALPARAMS)
set_target_properties(microbenchmark-bfv PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(microbenchmark-bfv SEAL::seal)

# Microbenchmark CKKS
add_executable(microbenchmark-ckks microbenchmark-ckks/microbenchmark.cpp common.h)
target_compile_definitions(microbenchmark-ckks PRIVATE SEALPARAMS)
set_target_properties(microbenchmark-ckks PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(microbenchmark-ckks SEAL::seal)

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


# Cardio batched BFV with default seal parameters
add_executable(cardio_bfv_batched_sealparams cardio-bfv-batched/cardio-batched.cpp common.h)
target_compile_definitions(cardio_bfv_batched_sealparams PRIVATE SEALPARAMS)
set_target_properties(cardio_bfv_batched_sealparams PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(cardio_bfv_batched_sealparams SEAL::seal)

# Cardio batched BFV with cinguparam parameters
add_executable(cardio_bfv_batched_cinguparam cardio-bfv-batched/cardio-batched.cpp common.h)
target_compile_definitions(cardio_bfv_batched_cinguparam PRIVATE CINGUPARAM)
set_target_properties(cardio_bfv_batched_cinguparam PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(cardio_bfv_batched_cinguparam SEAL::seal)

# Cardio batched BFV with manual parameters
add_executable(cardio_bfv_batched_manualparams cardio-bfv-batched/cardio-batched.cpp common.h)
target_compile_definitions(cardio_bfv_batched_manualparams PRIVATE MANUALPARAMS)
set_target_properties(cardio_bfv_batched_manualparams PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(cardio_bfv_batched_manualparams SEAL::seal)

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

# Chi Squared BFV OPT (MANUAl PARAMS)
add_executable(chi_squared_opt chi-squared-bfv-opt/chi_squared.cpp)
target_compile_definitions(chi_squared_opt PRIVATE MANUALPARAMS)
set_target_properties(chi_squared_opt PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(chi_squared_opt SEAL::seal)

# Chi Squared BFV NAIVE (SEAL PARAMS)
add_executable(chi_squared_naive chi-squared-bfv-naive/chi_squared.cpp)
target_compile_definitions(chi_squared_naive PRIVATE SEALPARAMS)
set_target_properties(chi_squared_naive PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(chi_squared_naive SEAL::seal)

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
