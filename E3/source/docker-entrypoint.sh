#!/bin/bash
set -euo pipefail
shopt -s inherit_errexit

export E3=/e3
export E3_SRC=/e3/src
export MAKEFLAGS=-j$(nproc)

upload_files() {
    TARGET_DIR=$1
    shift
    for item in "$@"; do
        aws s3 cp $item ${S3_URL}/${S3_FOLDER}/${TARGET_DIR}/
    done  
}

get_timestamp_ms() {
  echo $(date +%s%3N)
}

# The workflow for building used here is described in section "manual building"
# at https://github.com/momalab/e3/tree/master/doc#manual-build-using-cgtexe.
# PROGRAM_DIR=/e3/eval/chi-squared
# echo "Running demo program 'fibo' with TFHE"
# cd ${E3_SRC}
# make SEAL=1 cgt.exe
# ./cgt.exe gen -c ${PROGRAM_DIR}/cgt.cfg -d ${PROGRAM_DIR}
# ./amalgam.sh
# g++ -I./ -I${PROGRAM_DIR} cgtshared.cpp secint.cpp ${PROGRAM_DIR}/*.cpp -o ./bob.exe
# echo "Running computation..."
# ./bob.exe
# echo "Decrypting result..."
# make alice USER=${PROGRAM_DIR}
# ./alice.exe

###############
# TFHE Programs
###############

# CARDIO
export OUT_FILENAME=e3_tfhe_cardio.csv
export CGT_FILENAME=cgt_tfhe.cfg

cd $E3/src && \
    make cleanall && \
    make TFHE=1

cd $E3/eval/cardio
echo "t_keygen,t_input_encryption,t_computation,t_decryption" > $OUT_FILENAME

cp $E3/src/e3int.h ./ 
cp $E3/src/cgtshared.* ./

START_KEYGEN_T=$( get_timestamp_ms )
$E3/src/cgt.exe gen -c ${CGT_FILENAME} -r $E3/src
END_KEYGEN_T=$( get_timestamp_ms )
echo -ne "$((${END_KEYGEN_T}-${START_KEYGEN_T}))," >> $OUT_FILENAME

g++ -std=c++17 -I$E3/3p/tfhe_unx/inc/tfhe -I$E3/3p/tfhe_unx/inc/fftwa -I$E3/3p/tfhe_unx/inc/fftw3 main.cpp secint.cpp cgtshared.cpp -o bob.exe $E3/3p/tfhe_unx/target/libtfhe.a $E3/3p/tfhe_unx/target/libfftw3.a
echo -ne "0," >> $OUT_FILENAME

START_COMPUTATION_T=$( get_timestamp_ms )
./bob.exe > output.tmp
END_COMPUTATION_T=$( get_timestamp_ms )
echo -ne "$((${END_COMPUTATION_T}-${START_COMPUTATION_T}))," >> $OUT_FILENAME

START_DECRYPTION_T=$( get_timestamp_ms )
echo "Result:" $($E3/src/cgt.exe dec -c ${CGT_FILENAME} -f output.tmp)
END_DECRYPTION_T=$( get_timestamp_ms )
echo "$((${END_DECRYPTION_T}-${START_DECRYPTION_T}))" >> $OUT_FILENAME

upload_files E3-TFHE ${OUTPUT_FILENAME}

###############
# SEAL Programs
###############

# CARDIO
export OUT_FILENAME=e3_seal_cardio.csv
export CGT_FILENAME=cgt_seal.cfg

cd $E3/src && \
    make cleanall && \
    make SEAL=1

cd $E3/eval/cardio
echo "t_keygen,t_input_encryption,t_computation,t_decryption" > $OUT_FILENAME

cp $E3/src/e3int.h ./
cp $E3/src/cgtshared.* ./

START_KEYGEN_T=$( get_timestamp_ms )
$E3/src/cgt.exe gen -c ${CGT_FILENAME} -r $E3/src
END_KEYGEN_T=$( get_timestamp_ms )
echo -ne "$((${END_KEYGEN_T}-${START_KEYGEN_T}))," >> $OUT_FILENAME

g++ -std=c++17 -I${E3}/3p/seal_unx/include main.cpp secint.cpp cgtshared.cpp -o bob.exe ${E3}/3p/seal_unx/native/libseal.a
echo -ne "0," >> $OUT_FILENAME

START_COMPUTATION_T=$( get_timestamp_ms )
./bob.exe > output.tmp
END_COMPUTATION_T=$( get_timestamp_ms )
echo -ne "$((${END_COMPUTATION_T}-${START_COMPUTATION_T}))," >> $OUT_FILENAME

START_DECRYPTION_T=$( get_timestamp_ms )
echo "Result:" $($E3/src/cgt.exe dec -c ${CGT_FILENAME} -f output.tmp)
END_DECRYPTION_T=$( get_timestamp_ms )
echo "$((${END_DECRYPTION_T}-${START_DECRYPTION_T}))" >> $OUT_FILENAME

upload_files E3-TFHE ${OUTPUT_FILENAME}
