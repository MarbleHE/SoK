#!/bin/bash
set -euo pipefail
shopt -s inherit_errexit

export E3=/e3
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

# Usage: Define label below as "batched:"
# function jumpto {
#     label=$1
#     cmd=$(sed -n "/$label:/{:a;n;p;ba};" $0 | grep -v ':$')
#     eval "$cmd"
#     exit
# }
# Uncomment following two lines:
# myLabel=${1:-"myLabel"}
# jumpto $myLabel

# The workflow for building used here is described in section "manual building"
# at https://github.com/momalab/e3/tree/master/doc#manual-build-using-cgtexe.
# PROGRAM_DIR=/e3/eval/chi-squared
# echo "Running demo program 'fibo' with TFHE"
# cd ${E3}/src
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

cd $E3/src && \
   make cleanall && \
   make TFHE=1 -j$(nproc)


echo "============================================================"
echo "==== Running cardio-tfhe ==================================="
echo "============================================================"

export OUT_FILENAME=e3_tfhe_cardio.csv
export CGT_FILENAME=cgt_tfhe.cfg

cd $E3/eval/cardio-tfhe
echo "t_keygen,t_input_encryption,t_computation,t_decryption" > $OUT_FILENAME

RUN=1
while (( $RUN <= $NUM_RUNS ))
do
   RUN=$(( $RUN + 1))
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
done

upload_files E3-TFHE ${OUT_FILENAME}

echo "============================================================"
echo "==== Running chi-squared-tfhe =============================="
echo "============================================================"

export OUT_FILENAME=e3_tfhe_chi_squared.csv
export CGT_FILENAME=cgt_tfhe.cfg

cd $E3/eval/chi-squared-tfhe
echo "t_keygen,t_input_encryption,t_computation,t_decryption" > $OUT_FILENAME

RUN=1
while (( $RUN <= $NUM_RUNS ))
do
   RUN=$(( $RUN + 1))
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
done

upload_files E3-TFHE ${OUT_FILENAME}


###############
# SEAL Programs
###############

cd $E3/src && \
    make cleanall && \
    make SEAL=1 -j$(nproc)

# myLabel:
echo "============================================================"
echo "==== Running kernel-seal ==================================="
echo "============================================================"

export OUT_FILENAME=e3_seal_kernel.csv
export CGT_FILENAME=cgt_seal.cfg

cd $E3/eval/kernel-seal
rm -f *.key secint.* *.temp *.tmp cgtshared.* *.exe e3int.* *.csv

echo "t_keygen,t_input_encryption,t_computation,t_decryption" > $OUT_FILENAME

RUN=1
while (( $RUN <= $NUM_RUNS ))
do
    RUN=$(( $RUN + 1))
    cp $E3/src/e3int.h ./
    cp $E3/src/cgtshared.* ./

    START_KEYGEN_T=$( get_timestamp_ms )
    $E3/src/cgt.exe gen -c ${CGT_FILENAME} -r $E3/src
    END_KEYGEN_T=$( get_timestamp_ms )
    echo -ne "$((${END_KEYGEN_T}-${START_KEYGEN_T}))," >> $OUT_FILENAME

    g++ -std=c++17 -Wno-deprecated-declarations -I${E3}/3p/seal_unx/include main.cpp secint.cpp cgtshared.cpp -o bob.exe ${E3}/3p/seal_unx/native/libseal.a
    echo -ne "0," >> $OUT_FILENAME

    START_COMPUTATION_T=$( get_timestamp_ms )
    ./bob.exe > output.tmp
    END_COMPUTATION_T=$( get_timestamp_ms )
    echo -ne "$((${END_COMPUTATION_T}-${START_COMPUTATION_T}))," >> $OUT_FILENAME

    START_DECRYPTION_T=$( get_timestamp_ms )
    echo -e "Result:\n" $($E3/src/cgt.exe dec -c ${CGT_FILENAME} -f output.tmp)
    END_DECRYPTION_T=$( get_timestamp_ms )
    echo "$((${END_DECRYPTION_T}-${START_DECRYPTION_T}))" >> $OUT_FILENAME
done

upload_files E3-SEAL ${OUT_FILENAME}

exit 0;

echo "============================================================"
echo "==== Running cardio-seal ==================================="
echo "============================================================"

export OUT_FILENAME=e3_seal_cardio.csv
export CGT_FILENAME=cgt_seal.cfg

cd $E3/eval/cardio-seal
echo "t_keygen,t_input_encryption,t_computation,t_decryption" > $OUT_FILENAME

RUN=1
while (( $RUN <= $NUM_RUNS ))
do
    RUN=$(( $RUN + 1))
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
done

upload_files E3-SEAL ${OUT_FILENAME}


echo "============================================================"
echo "==== Running cardio-seal-batched ==========================="
echo "============================================================"

export OUT_FILENAME=e3_seal_batched_cardio.csv
export CGT_FILENAME=cgt_seal.cfg

cd $E3/eval/cardio-seal-batched
echo "t_keygen,t_input_encryption,t_computation,t_decryption" > $OUT_FILENAME

RUN=1
while (( $RUN <= $NUM_RUNS ))
do
    RUN=$(( $RUN + 1))
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
done

upload_files E3-SEAL-Batched ${OUT_FILENAME}

echo "============================================================"
echo "==== Running chi-squared-seal =============================="
echo "============================================================"

export OUT_FILENAME=e3_seal_chi_squared.csv
export CGT_FILENAME=cgt_seal.cfg

cd $E3/eval/chi-squared-seal
echo "t_keygen,t_input_encryption,t_computation,t_decryption" > $OUT_FILENAME

RUN=1
while (( $RUN <= $NUM_RUNS ))
do
    RUN=$(( $RUN + 1))
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
done

upload_files E3-SEAL ${OUT_FILENAME}
