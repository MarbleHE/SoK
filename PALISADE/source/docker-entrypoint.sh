#!/bin/bash

function upload_files() {
    TARGET_DIR=$1
    shift
    for item in "$@"; do
        aws s3 cp $item ${S3_URL}/${S3_FOLDER}/${TARGET_DIR}/
    done  
}

# Build benchmark programs
cd /root/eval \
    && mkdir build \
    && cd build \
    && cmake .. \
    && make

# Run microbenchmarks
export OUTPUT_FILENAME=palisade_microbenchmark-bfv-bgv.csv
./microbenchmark-bfv-bgv
upload_files PALISADE-BFV-BGV ${OUTPUT_FILENAME} fhe_parameters_microbenchmark_bfv_bgv.txt

export OUTPUT_FILENAME=palisade_microbenchmark-ckks.csv
./microbenchmark-ckks
upload_files PALISADE-CKKS ${OUTPUT_FILENAME} fhe_parameters_microbenchmark_ckks.txt
