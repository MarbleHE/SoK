# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT license.

add_executable(runtime)

target_sources(runtime PRIVATE runtime.cpp)

target_link_libraries(runtime PRIVATE eva)