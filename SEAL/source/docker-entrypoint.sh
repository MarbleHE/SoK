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

# cardio BFV
./run-cardio-bfv.sh
upload_files SEAL-BFV seal_bfv_cardio.csv fhe_parameters.txt

# cardio BFV batched
./run-cardio-bfv-batched.sh
upload_files SEAL-BFV-Batched seal_batched_bfv_cardio.csv fhe_parameters.txt

# cardio CKKS batched
./run-cardio-ckks-batched.sh
upload_files SEAL-CKKS-Batched seal_batched_ckks_cardio.csv fhe_parameters.txt

# NN batched
./run-nn-ckks-batched.sh
upload_files SEAL-CKKS-Batched seal_batched_nn.csv fhe_parameters.txt
