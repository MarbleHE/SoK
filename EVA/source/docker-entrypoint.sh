#!/bin/bash

EVA_DIR=/EVA
cd $EVA_DIR || (echo "Could not switch to ${EVA_DIR}" && exit)

upload_files() {
    TARGET_DIR=$1
    shift
    for item in "$@"; do
        aws s3 cp $item ${S3_URL}/${S3_FOLDER}/${TARGET_DIR}/
    done
}


function run_cpp_benchmark() {
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
        /EVA/runtime/runtime $2
    done
}


# MLP
export OUTPUT_FILENAME=eva_mlp_nn.csv
run_cpp_benchmark runtime /root/eval/nn-mlp/mlp
upload_files EVA-MLP ${OUTPUT_FILENAME}

# LeNet-5
export OUTPUT_FILENAME=eva_lenet5_nn.csv
run_cpp_benchmark runtime /root/eval/nn-lenet5/small_mnist_hw
upload_files EVA-LeNet5 ${OUTPUT_FILENAME}

## Run example program: image_processing
#export OUTPUT_FILENAME=eva_kernel.csv
#cd /root/eval/image_processing \
#    && python3 -m pip install -r requirements.txt \
#    && python3 image_processing.py
#upload_files EVA ${OUTPUT_FILENAME}

# Run benchmark program: chi_squared
export OUTPUT_FILENAME=eva_chi_squared.csv
cd /root/eval/chi_squared \
    && python3 -m pip install -r requirements.txt \
    && python3 chi_squared.py
upload_files EVA ${OUTPUT_FILENAME}
