cmake_minimum_required(VERSION 3.0)

set(PROG_NAME bfv-cardio-lobster)

set(SRCS cardio.cxx)
set(BLOP_NAME cardio_lobster.blif)

#add_compile_options(-Dblif_name="${BLOP_NAME}")

# set(GEN_NAME ${PROG_NAME}-gen)
# add_executable(${GEN_NAME} ${SRCS})
# target_link_libraries(${GEN_NAME} common)

if (USE_BFV)
  if(NOT DEFINED MODEL)
    set(MODEL bkz_sieve)
  endif()

  if(NOT DEFINED MIN_SECU)
    set(MIN_SECU 128)
  endif()
  if(NOT DEFINED POLITIC) 
        set(POLITIC Cingulata_BFV)
  endif()
endif(USE_BFV)

set(XML_PARAMS fhe_params.xml)
set(MUL_DEPTH_SCRIPT ${OPTIM_DIR}/graph_info.py)

# copy circuit file to build directory so that selectParams.sh can find it
file(COPY ${BLOP_NAME} DESTINATION .)

# copy circuit input file to build directory
file(COPY clear_data.data DESTINATION .)

add_custom_command(OUTPUT ${XML_PARAMS}
        COMMAND bash ${SCRIPT_DIR}/selectParams.sh ${PROG_NAME} `python3 ${MUL_DEPTH_SCRIPT} ${BLOP_NAME} --mult_depth_max` ${MODEL} ${MIN_SECU} ${POLITIC}
        DEPENDS ${BLOP_NAME})

add_custom_target(${PROG_NAME} ALL
  DEPENDS ${XML_PARAMS} runtime)

set(APPS_DIR ${CMAKE_BINARY_DIR}/apps)
set(OPTIMIZED_CIRCUIT ${BLOP_NAME})

# configure file and write into temporary directory
configure_file(run.sh.in ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/run.sh @ONLY)
# move file into final directory and make it executable
file(COPY ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/run.sh
  DESTINATION .
  FILE_PERMISSIONS 
    OWNER_READ OWNER_WRITE OWNER_EXECUTE 
    GROUP_READ GROUP_EXECUTE 
    WORLD_READ WORLD_EXECUTE)
