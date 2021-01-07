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

# Cardio Opt
export OUTPUT_FILENAME=tfhe_cardio_opt.csv
./run_cardio_opt.sh
upload_files TFHE-Opt ${OUTPUT_FILENAME}

# Cardio Naive
export OUTPUT_FILENAME=tfhe_cardio_naive.csv
./run_cardio_naive.sh
upload_files TFHE-Naive ${OUTPUT_FILENAME}

# Chi-Squared Naive
export OUTPUT_FILENAME=tfhe_chi_squared_naive.csv
./run_chi_squared_naive.sh
upload_files TFHE-Naive ${OUTPUT_FILENAME}

# Chi-Squared Opt
export OUTPUT_FILENAME=tfhe_chi_squared_opt.csv
./run_chi_squared_opt.sh
upload_files TFHE-Opt ${OUTPUT_FILENAME}