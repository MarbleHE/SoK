cmake_minimum_required(VERSION 3.9)

add_executable(tfhe-chi-squared-exec chi-squared-tfhe.cxx)
target_link_libraries(tfhe-chi-squared-exec common tfhe_bit_exec)

add_executable(tfhe-chi-squared-encrypt encrypt.cxx)
target_link_libraries(tfhe-chi-squared-encrypt common tfhe_bit_exec)

add_executable(tfhe-chi-squared-decrypt decrypt.cxx)
target_link_libraries(tfhe-chi-squared-decrypt common tfhe_bit_exec)

add_custom_target(tfhe-chi-squared
  DEPENDS
    tfhe-chi-squared-exec
    tfhe-chi-squared-encrypt
    tfhe-chi-squared-decrypt
    tfhe
)

set(APPS_DIR ${CMAKE_BINARY_DIR}/apps)
configure_file("run.sh.in" "run.sh" @ONLY)
