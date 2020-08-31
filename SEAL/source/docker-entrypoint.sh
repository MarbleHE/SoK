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

# Cardio BFV with CinguParam parameters
export OUTPUT_FILENAME=seal_bfv_cardio.csv
./run-cardio-bfv.sh
upload_files SEAL-BFV ${OUTPUT_FILENAME} fhe_parameters.txt

# Cardio BFV with optimal parameters
export OUTPUT_FILENAME=seal_bfv_cardio_optimal.csv
./run-cardio-bfv-opt.sh
upload_files SEAL-BFV-OPT ${OUTPUT_FILENAME} fhe_parameters.txt

# cardio BFV batched
export OUTPUT_FILENAME=seal_batched_bfv_cardio.csv
./run-cardio-bfv-batched.sh
upload_files SEAL-BFV-Batched ${OUTPUT_FILENAME} fhe_parameters.txt

# cardio CKKS batched
export OUTPUT_FILENAME=seal_batched_ckks_cardio.csv
./run-cardio-ckks-batched.sh
upload_files SEAL-CKKS-Batched ${OUTPUT_FILENAME} fhe_parameters_cardio.txt

# NN batched
export OUTPUT_FILENAME=seal_batched_ckks_nn.csv
./run-nn-ckks-batched.sh
upload_files SEAL-CKKS-Batched ${OUTPUT_FILENAME} fhe_parameters_nn.txt

# Chi Squared BFV
export OUTPUT_FILENAME=seal_bfv_chi_squared.csv
./run-chi-squared-bfv.sh
upload_files SEAL-BFV ${OUTPUT_FILENAME} fhe_parameters_chi_squared.txt
