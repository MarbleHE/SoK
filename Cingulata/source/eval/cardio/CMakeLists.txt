cmake_minimum_required(VERSION 3.0)

set(PROG_NAME bfv-cardio)

set(SRCS cardio.cxx)
set(BLIF_NAME ${PROG_NAME}.blif)
set(BLOP_NAME ${PROG_NAME}-opt.blif)

add_compile_options(-Dblif_name="${BLIF_NAME}")

set(GEN_NAME ${PROG_NAME}-gen)

add_executable(${GEN_NAME} ${SRCS})

target_link_libraries(${GEN_NAME} common)

add_custom_command(OUTPUT ${BLIF_NAME}
  COMMAND ./${GEN_NAME}
  DEPENDS ${GEN_NAME})
  
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

add_custom_command(OUTPUT ${BLOP_NAME}
  COMMAND python3 ${OPTIM_DIR}/abc_optimize.py -i ${BLIF_NAME} -o ${BLOP_NAME} -v
  DEPENDS abc ${BLIF_NAME})

set(XML_PARAMS fhe_params.xml)
set(MUL_DEPTH_SCRIPT ${OPTIM_DIR}/graph_info.py)

add_custom_command(OUTPUT ${XML_PARAMS}
        COMMAND bash ${SCRIPT_DIR}/selectParams.sh ${PROG_NAME} `python3 ${MUL_DEPTH_SCRIPT} ${BLOP_NAME} --mult_depth_max` ${MODEL} ${MIN_SECU} ${POLITIC}
        DEPENDS ${BLOP_NAME})

add_custom_target(${PROG_NAME} ALL
  DEPENDS ${XML_PARAMS} runtime)

set(APPS_DIR ${CMAKE_BINARY_DIR}/apps)
set(UNOPTIMIZED_CIRCUIT ${BLIF_NAME})
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
