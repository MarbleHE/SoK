cmake_minimum_required (VERSION 3.9...3.17)

project(eval_benchmark)

find_package(SEAL 3.5 CONFIG REQUIRED)

set(CMAKE_BUILD_TYPE DEBUG)

# target_link_libraries(main PRIVATE SEAL::seal MSGSL::MSGSL)

# Cardio BFV with CinguParam parameters
add_executable(cardio_bfv cardio-bfv/cardio.cpp common.h)
set_target_properties(cardio_bfv PROPERTIES LINKER_LANGUAGE CXX) 
target_link_libraries(cardio_bfv SEAL::seal)
configure_file(cardio-bfv/run-cardio-bfv.sh.in tmp/run-cardio-bfv.sh)
file (COPY ${CMAKE_BINARY_DIR}/tmp/run-cardio-bfv.sh DESTINATION ${CMAKE_BINARY_DIR} FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ)

# Cardio BFV with optimal parameters
add_executable(cardio_bfv_opt cardio-bfv-OPT-PARAMS/cardio.cpp common.h)
set_target_properties(cardio_bfv_opt PROPERTIES LINKER_LANGUAGE CXX) 
target_link_libraries(cardio_bfv_opt SEAL::seal)
configure_file(cardio-bfv-OPT-PARAMS/run-cardio-bfv-opt.sh.in tmp/run-cardio-bfv-opt.sh)
file (COPY ${CMAKE_BINARY_DIR}/tmp/run-cardio-bfv-opt.sh DESTINATION ${CMAKE_BINARY_DIR} FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ)

# Cardio batched BFV
add_executable(cardio_bfv_batched cardio-bfv-batched/cardio-batched.cpp common.h)
set_target_properties(cardio_bfv_batched PROPERTIES LINKER_LANGUAGE CXX) 
target_link_libraries(cardio_bfv_batched SEAL::seal)
configure_file(cardio-bfv-batched/run-cardio-bfv-batched.sh.in tmp/run-cardio-bfv-batched.sh)
file (COPY ${CMAKE_BINARY_DIR}/tmp/run-cardio-bfv-batched.sh DESTINATION ${CMAKE_BINARY_DIR} FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ)

# Cardio batched CKKS
add_executable(cardio_ckks_batched cardio-ckks-batched/cardio-batched.cpp common.h)
set_target_properties(cardio_ckks_batched PROPERTIES LINKER_LANGUAGE CXX) 
target_link_libraries(cardio_ckks_batched SEAL::seal)
configure_file(cardio-ckks-batched/run-cardio-ckks-batched.sh.in tmp/run-cardio-ckks-batched.sh)
file (COPY ${CMAKE_BINARY_DIR}/tmp/run-cardio-ckks-batched.sh DESTINATION ${CMAKE_BINARY_DIR} FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ)

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
configure_file(nn-ckks-batched/run-nn-ckks-batched.sh.in tmp/run-nn-ckks-batched.sh)
file (COPY ${CMAKE_BINARY_DIR}/tmp/run-nn-ckks-batched.sh DESTINATION ${CMAKE_BINARY_DIR} FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ)

# NN batched CKKS tests
# add_subdirectory(nn-ckks-batched/tests)

# Chi Squared BFV
add_executable(chi_squared chi-squared-bfv/chi_squared.cpp)
set_target_properties(chi_squared PROPERTIES LINKER_LANGUAGE CXX) 
target_link_libraries(chi_squared SEAL::seal)
configure_file(chi-squared-bfv/run-chi-squared-bfv.sh.in tmp/run-chi-squared-bfv.sh)
file (COPY ${CMAKE_BINARY_DIR}/tmp/run-chi-squared-bfv.sh DESTINATION ${CMAKE_BINARY_DIR} FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ)

# Chi Squared BFV Batched
add_executable(chi_squared_batched chi-squared-bfv-batched/chi_squared_batched.cpp)
set_target_properties(chi_squared_batched PROPERTIES LINKER_LANGUAGE CXX) 
target_link_libraries(chi_squared_batched SEAL::seal)
configure_file(chi-squared-bfv-batched/run-chi-squared-bfv-batched.sh.in tmp/run-chi-squared-bfv-batched.sh)
file (COPY ${CMAKE_BINARY_DIR}/tmp/run-chi-squared-bfv-batched.sh DESTINATION ${CMAKE_BINARY_DIR} FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ)
