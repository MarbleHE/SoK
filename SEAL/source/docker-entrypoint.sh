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

# Cardio BFV (using modified Cingulata parameters)
export OUTPUT_FILENAME=seal_bfv_cardio_cinguparam.csv
run_benchmark cardio_bfv_cinguparam
upload_files SEAL-BFV-Cinguparam ${OUTPUT_FILENAME} fhe_parameters_cardio.txt

# Cardio BFV (using Seal's automatically determined moduli)
export OUTPUT_FILENAME=seal_bfv_cardio_sealparams.csv
run_benchmark cardio_bfv_sealparams
upload_files SEAL-BFV-Sealparams ${OUTPUT_FILENAME} fhe_parameters_cardio.txt

# Cardio BFV (using manually determined parameters)
export OUTPUT_FILENAME=seal_bfv_cardio_manaulparams.csv
run_benchmark cardio_bfv_manualparams
upload_files SEAL-BFV-Manualparams ${OUTPUT_FILENAME} fhe_parameters_cardio.txt


# Cardio BFV Naive with seal default params
export OUTPUT_FILENAME=seal_bfv_cardio_naive_sealparams.csv
run_benchmark cardio_bfv_naive_sealparams
upload_files SEAL-BFV-Naive-Sealparams ${OUTPUT_FILENAME} fhe_parameters_cardio.txt

# Cardio BFV Naive with same manually selected params as manualparams
export OUTPUT_FILENAME=seal_bfv_cardio_naive_manualparams.csv
run_benchmark cardio_bfv_naive_manualparams
upload_files SEAL-BFV-Naive-Manualparams ${OUTPUT_FILENAME} fhe_parameters_cardio.txt

# Cardio BFV Naive with cinguparam parameters
export OUTPUT_FILENAME=seal_bfv_cardio_naive_cinguparam.csv
run_benchmark cardio_bfv_naive_cinguparam
upload_files SEAL-BFV-Naive-Cinguparam ${OUTPUT_FILENAME} fhe_parameters_cardio.txt

# Cardio BFV batched with seal default params
export OUTPUT_FILENAME=seal_batched_bfv_cardio_sealparams.csv
run_benchmark cardio_bfv_batched_sealparams
upload_files SEAL-BFV-Batched-Sealparams ${OUTPUT_FILENAME} fhe_parameters_cardio.txt

# Cardio BFV batched with cinguparam
export OUTPUT_FILENAME=seal_batched_bfv_cardio_cingupara.csv
run_benchmark cardio_bfv_batched_cinguparam
upload_files SEAL-BFV-Batched-Cinguparam ${OUTPUT_FILENAME} fhe_parameters_cardio.txt

# Cardio BFV batched with manual params
export OUTPUT_FILENAME=seal_batched_bfv_cardio_manualparams.csv
run_benchmark cardio_bfv_batched_manualparams
upload_files SEAL-BFV-Batched-Manualparams ${OUTPUT_FILENAME} fhe_parameters_cardio.txt

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

# Chi-Squared BFV Batched
export OUTPUT_FILENAME=seal_bfv_batched_chi_squared.csv
run_benchmark chi_squared_batched
upload_files SEAL-BFV-Batched ${OUTPUT_FILENAME} fhe_parameters_chi_squared.txt

# Kernel BFV
export OUTPUT_FILENAME=seal_bfv_kernel.csv
run_benchmark kernel
upload_files SEAL-BFV ${OUTPUT_FILENAME} fhe_parameters_kernel.txt

# Kernel BFV batched
export OUTPUT_FILENAME=seal_batched_bfv_kernel.csv
run_benchmark kernel_batched
upload_files SEAL-BFV-Batched ${OUTPUT_FILENAME} fhe_parameters_kernel.txt
