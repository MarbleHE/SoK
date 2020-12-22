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

# Run microbenchmark
./microbenchmark
export OUTPUT_FILENAME=palisade_microbenchmark.csv
upload_files PALISADE-BFV ${OUTPUT_FILENAME} fhe_parameters_microbenchmark.txt
