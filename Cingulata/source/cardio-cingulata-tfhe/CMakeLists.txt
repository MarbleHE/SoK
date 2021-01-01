cmake_minimum_required(VERSION 3.9)

add_executable(tfhe-cardio-exec cardio-tfhe.cxx)
target_link_libraries(tfhe-cardio-exec common tfhe_bit_exec)

add_executable(tfhe-cardio-encrypt encrypt.cxx)
target_link_libraries(tfhe-cardio-encrypt common tfhe_bit_exec)

add_executable(tfhe-cardio-decrypt decrypt.cxx)
target_link_libraries(tfhe-cardio-decrypt common tfhe_bit_exec)

add_custom_target(tfhe-cardio
  DEPENDS
    tfhe-cardio-exec
    tfhe-cardio-encrypt
    tfhe-cardio-decrypt
    tfhe
)

set(APPS_DIR ${CMAKE_BINARY_DIR}/apps)
configure_file("run.sh.in" "run.sh" @ONLY)
