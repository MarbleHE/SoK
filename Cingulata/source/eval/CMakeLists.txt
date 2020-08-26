cmake_minimum_required(VERSION 3.9)

set(OPTIM_DIR ${cingulata_BINARY_DIR}/optim)
set(SCRIPT_DIR ${cingulata_BINARY_DIR}/runtime/fhe_fv/script)

set(benchmarks
      cardio
      cardio-lobster
)

set(target_names "")
foreach (name IN ITEMS ${benchmarks})
  add_subdirectory(${name})
  list(APPEND target_names "bfv-${name}")
endforeach()

add_custom_target(eval_bfv ALL
  DEPENDS
    ${target_names}
)
