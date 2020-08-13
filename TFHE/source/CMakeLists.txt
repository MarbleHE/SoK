cmake_minimum_required (VERSION 3.9...3.17)


project(eval_benchmark)

set(CMAKE_CXX_STANDARD 17)



# Cardio
add_executable(cardio cardio/cardio.cpp)
set_target_properties(cardio PROPERTIES LINKER_LANGUAGE CXX) 
target_link_libraries(cardio /usr/local/lib/libtfhe-spqlios-fma.so)
configure_file(cardio/run.sh.in tmp/run.sh COPYONLY)
file (COPY ${CMAKE_BINARY_DIR}/tmp/run.sh DESTINATION ${CMAKE_BINARY_DIR} FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ)
