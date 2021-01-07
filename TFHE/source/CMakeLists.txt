cmake_minimum_required (VERSION 3.9...3.17)

project(eval_benchmark)

# Cardio Naive
add_executable(cardio-naive cardio-naive/cardio.cpp)
set_target_properties(cardio-naive PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(cardio-naive /usr/local/lib/libtfhe-fftw.so)
configure_file(cardio-naive/run_cardio.sh.in tmp/run_cardio_naive.sh)
file (COPY ${CMAKE_BINARY_DIR}/tmp/run_cardio_naive.sh DESTINATION ${CMAKE_BINARY_DIR} FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ)


# Cardio Opt
add_executable(cardio-opt cardio-opt/cardio.cpp)
set_target_properties(cardio-opt PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(cardio-opt /usr/local/lib/libtfhe-fftw.so)
configure_file(cardio-opt/run_cardio.sh.in tmp/run_cardio_opt.sh)
file (COPY ${CMAKE_BINARY_DIR}/tmp/run_cardio_opt.sh DESTINATION ${CMAKE_BINARY_DIR} FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ)


# Chi-Squared Test
add_executable(chi_squared chi-squared/chi-squared.cpp)
set_target_properties(chi_squared PROPERTIES LINKER_LANGUAGE CXX) 
target_link_libraries(chi_squared /usr/local/lib/libtfhe-fftw.so)
configure_file(chi-squared/run_chi_squared.sh.in tmp/run_chi_squared.sh)
file (COPY ${CMAKE_BINARY_DIR}/tmp/run_chi_squared.sh DESTINATION ${CMAKE_BINARY_DIR} FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ)
