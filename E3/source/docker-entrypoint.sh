#!/bin/bash

E3_ROOT=/e3/src
export MAKEFLAGS=-j`nproc`

# The workflow for building used here is described in section "manual building"
# at https://github.com/momalab/e3/tree/master/doc#manual-build-using-cgtexe.
PROGRAM_DIR=/e3/eval/fibo
echo "Running demo program 'fibo' with TFHE"
cd ${E3_ROOT}
make cgt.exe
./cgt.exe gen -c ${PROGRAM_DIR}/cgt.cfg -d ${PROGRAM_DIR}
./amalgam.sh
g++ -I./ -I${PROGRAM_DIR} cgtshared.cpp secint.cpp ${PROGRAM_DIR}/*.cpp -o ./bob.exe
echo "Running computation..."
./bob.exe
echo "Decrypting result..."
make alice USER=${PROGRAM_DIR}
./alice.exe
