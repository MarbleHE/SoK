cmake_minimum_required (VERSION 3.5.1)

project(demo CXX)
set(CMAKE_CXX_STANDARD 11)

find_package(Palisade)

set( CMAKE_CXX_FLAGS ${PALISADE_CXX_FLAGS} )
set( CMAKE_EXE_LINKER_FLAGS ${PALISADE_EXE_LINKER_FLAGS} )

include_directories( ${OPENMP_INCLUDES} )
include_directories( ${PALISADE_INCLUDE} )
include_directories( ${PALISADE_INCLUDE}/third-party/include )
include_directories( ${PALISADE_INCLUDE}/core )
include_directories( ${PALISADE_INCLUDE}/pke )
include_directories( ${PALISADE_INCLUDE}/binfhe )
### add directories for other PALISADE modules as needed for your project

link_directories( ${PALISADE_LIBDIR} )
link_directories( ${OPENMP_LIBRARIES} )
link_libraries( ${PALISADE_LIBRARIES} )

### ADD YOUR EXECUTABLE(s) HERE
add_executable(microbenchmark-bfv-bgv microbenchmark-bfv-bgv/microbenchmark.cpp )
add_executable(microbenchmark-ckks microbenchmark-ckks/microbenchmark.cpp )
add_executable(microbenchmark-fhew microbenchmark-fhew/microbenchmark.cpp )
