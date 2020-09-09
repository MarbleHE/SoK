#!/bin/bash

EVAL_BUILD_DIR=/root/eval/build
cd $EVAL_BUILD_DIR || (echo "Could not switch to ${EVAL_BUILD_DIR}" && exit)

function upload_files() {
    TARGET_DIR=$1
    shift
    for item in "$@"; do
        aws s3 cp $item ${S3_URL}/${S3_FOLDER}/${TARGET_DIR}/
    done  
}

function run_benchmark() {
    cd $EVAL_BUILD_DIR
    echo "t_keygen,t_input_encryption,t_computation,t_decryption" > $OUTPUT_FILENAME
    RUN=1
    if [ -z "${NUM_RUNS}" ]
    then
        echo "Cannot continue as NUM_RUNS is undefined!"
        exit 1
    fi
    while (( $RUN <= $NUM_RUNS ))
    do
        RUN=$(( $RUN + 1))
        ./$1
    done
}


Cardio BFV (with Cingulata parameters)
export OUTPUT_FILENAME=seal_bfv_cardio_optimal.csv
run_benchmark cardio_bfv
upload_files SEAL-BFV ${OUTPUT_FILENAME} fhe_parameters.txt

# Cardio BFV (with optimal parameters)
export OUTPUT_FILENAME=seal_bfv_cardio_optimal.csv
run_benchmark cardio_bfv
upload_files SEAL-BFV-OPT ${OUTPUT_FILENAME} fhe_parameters.txt

# Cardio BFV batched
export OUTPUT_FILENAME=seal_batched_bfv_cardio.csv
run_benchmark cardio_bfv_batched
upload_files SEAL-BFV-Batched ${OUTPUT_FILENAME} fhe_parameters.txt

# Cardio CKKS batched
export OUTPUT_FILENAME=seal_batched_ckks_cardio.csv
run_benchmark cardio_ckks_batched
upload_files SEAL-CKKS-Batched ${OUTPUT_FILENAME} fhe_parameters_cardio.txt

# NN CKKS batched
export OUTPUT_FILENAME=seal_batched_ckks_nn.csv
run_benchmark nn_ckks_batched
upload_files SEAL-CKKS-Batched ${OUTPUT_FILENAME} fhe_parameters_nn.txt

# Chi-Squared BFV
export OUTPUT_FILENAME=seal_bfv_chi_squared.csv
run_benchmark chi_squared
upload_files SEAL-BFV ${OUTPUT_FILENAME} fhe_parameters_chi_squared.txt

Chi-Squared BFV Batched
export OUTPUT_FILENAME=seal_bfv_batched_chi_squared.csv
run_benchmark chi_squared_batched
upload_files SEAL-BFV-Batched ${OUTPUT_FILENAME} fhe_parameters_chi_squared.txt

# Kernel BFV
export OUTPUT_FILENAME=seal_bfv_kernel.csv
run_benchmark kernel
upload_files SEAL-BFV ${OUTPUT_FILENAME} fhe_parameters.txt

# Kernel BFV batched
export OUTPUT_FILENAME=seal_batched_bfv_kernel.csv
run_benchmark kernel_batched
upload_files SEAL-BFV-Batched ${OUTPUT_FILENAME} fhe_parameters.txt
