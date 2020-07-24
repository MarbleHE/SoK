find_path(Cingulata_INCLUDE_DIR NAMES ci_int.hxx PATHS /cingu/common/include)
find_library(Cingulata_LIBRARY NAMES common PATHS /cingu/build_bfv/common/src)


set(Cingulata_INCLUDE_DIRS ${Cingulata_INCLUDE_DIR})
set(Cingulata_LIBRARIES ${Cingulata_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Cingulata DEFAULT_MSG
        Cingulata_LIBRARIES Cingulata_INCLUDE_DIRS)

mark_as_advanced(Cingulata_LIBRARY Cingulata_INCLUDE_DIR)