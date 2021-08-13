cmake_minimum_required (VERSION 3.9...3.17)

project(eval_benchmark)

# Hamming Opt
add_executable(hamming-opt hamming-opt/hamming.cpp)
set_target_properties(hamming-opt PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(hamming-opt /usr/local/lib/libtfhe-fftw.so)
configure_file(hamming-opt/run_hamming.sh.in tmp/run_hamming_opt.sh)
file (COPY ${CMAKE_BINARY_DIR}/tmp/run_hamming_opt.sh DESTINATION ${CMAKE_BINARY_DIR} FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ)


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


# Chi-Squared Naive
add_executable(chi_squared_naive chi-squared-naive/chi-squared.cpp)
set_target_properties(chi_squared_naive PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(chi_squared_naive /usr/local/lib/libtfhe-fftw.so)
configure_file(chi-squared-naive/run_chi_squared.sh.in tmp/run_chi_squared_naive.sh)
file (COPY ${CMAKE_BINARY_DIR}/tmp/run_chi_squared_naive.sh DESTINATION ${CMAKE_BINARY_DIR} FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ)

# Chi-Squared Opt
add_executable(chi_squared_opt chi-squared-opt/chi-squared.cpp)
set_target_properties(chi_squared_opt PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(chi_squared_opt /usr/local/lib/libtfhe-fftw.so)
configure_file(chi-squared-opt/run_chi_squared.sh.in tmp/run_chi_squared_opt.sh)
file (COPY ${CMAKE_BINARY_DIR}/tmp/run_chi_squared_opt.sh DESTINATION ${CMAKE_BINARY_DIR} FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ)
