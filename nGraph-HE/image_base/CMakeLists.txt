# ******************************************************************************
# Copyright 2018-2020 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTNNPS OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
# ******************************************************************************

set(SRC
    main.cpp
    test_seal.cpp
    test_encryption_parameters.cpp
    # src/
    test_he_op_annotations.cpp
    test_he_plaintext.cpp
    test_he_tensor.cpp
    test_he_type.cpp
    test_he_util.cpp
    # src/pass
    test_he_fusion.cpp
    test_he_supported_ops.cpp
    test_propagate_he_annotations.cpp
    # src/seal
    test_encryption_parameters.cpp
    test_he_seal_executable.cpp
    test_bounded_relu.cpp
    test_perf_micro.cpp
    test_seal.cpp
    test_protobuf.cpp
    test_seal_plaintext_wrapper.cpp
    test_seal_util.cpp
    # src/tcp
    test_tcp_message.cpp
    test_tcp_client.cpp
    # test logging
    test_ngraph_he_log.cpp
    )

if (NGRAPH_HE_ABY_ENABLE)
  list(APPEND SRC test_aby.cpp test_aby_relu.cpp)
endif()

set(BACKEND_TEST_SRC
    test_add.in.cpp
    test_avg_pool.in.cpp
    test_batch_norm_inference.in.cpp
    test_basics.in.cpp
    test_broadcast.in.cpp
    test_concat.in.cpp
    test_constant.in.cpp
    test_convolution.in.cpp
    test_divide.in.cpp
    test_exp.in.cpp
    test_dot.in.cpp
    test_rescale.in.cpp
    test_layers.in.cpp
    test_maxpool.in.cpp
    test_minimum.in.cpp
    test_multiply.in.cpp
    test_max.in.cpp
    test_negate.in.cpp
    test_pad.in.cpp
    test_power.in.cpp
    test_read_write.in.cpp
    test_relu.in.cpp
    test_reshape.in.cpp
    test_result.in.cpp
    test_reverse.in.cpp
    test_server_client.in.cpp
    test_softmax.in.cpp
    test_slice.in.cpp
    test_subtract.in.cpp
    test_sum.in.cpp)

if (NGRAPH_HE_ABY_ENABLE)
  list(APPEND BACKEND_TEST_SRC test_server_client_gc.in.cpp)
endif()

set(MANIFEST ${PROJECT_SOURCE_DIR}/src/seal/unit_test.manifest)
set(BACKEND_NAME "HE_SEAL")

foreach(TEST_SRC ${BACKEND_TEST_SRC})
  string(REPLACE ".in."
                 "_${BACKEND_NAME}."
                 TARGET_NAME
                 ${TEST_SRC})
  configure_file(${TEST_SRC} ${TARGET_NAME})
  set(SRC ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME} ${SRC})
endforeach()

# Copy clang-tidy file to build directory
configure_file(.clang-tidy ${CMAKE_CURRENT_BINARY_DIR}/.clang-tidy COPYONLY)

message(STATUS "Adding unit test for backend ${BACKEND_NAME}")


# Clang passing flags to the linker (likely ld) AND using -v itself to show how it calls the linker
set(CMAKE_EXE_LINKER_FLAGS "-Xlinker -v -v")

add_executable(unit-test ${SRC})

# Verbose compilation steps
target_compile_options(unit-test PRIVATE -v)

target_include_directories(unit-test PRIVATE ".")

target_link_libraries(unit-test PUBLIC
                he_seal_backend
                libabseil
                libboost
                libgtest 
                libjson
                libngraph
                libngraph_test_util
                libseal
                protobuf::libprotobuf
                pthread
                zlib)


if (NGRAPH_HE_ABY_ENABLE)
  target_link_libraries(unit-test PRIVATE libaby)
endif()

add_custom_target(check
                  COMMAND ${PROJECT_BINARY_DIR}/test/unit-test \${ARGS}
                  DEPENDS unit-test)
