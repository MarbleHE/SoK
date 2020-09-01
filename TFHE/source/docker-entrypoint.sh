#!/bin/bash

upload_files() {
    TARGET_DIR=$1
    shift
    for item in "$@"; do
        aws s3 cp $item ${S3_URL}/${S3_FOLDER}/${TARGET_DIR}/
    done  
}

EVAL_BUILD_DIR=/root/eval/build
cd $EVAL_BUILD_DIR || exit

# Cardio 
export OUTPUT_FILENAME=tfhe_cardio.csv
./run_cardio.sh
upload_files TFHE ${OUTPUT_FILENAME}

# Chi-Squared Test
export OUTPUT_FILENAME=tfhe_chi_squared.csv
./run_chi_squared.sh
upload_files TFHE ${OUTPUT_FILENAME}
