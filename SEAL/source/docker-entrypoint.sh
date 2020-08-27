#!/bin/bash

EVAL_BUILD_DIR=/root/eval/build
cd $EVAL_BUILD_DIR || exit

# cardio BFV
./run-cardio-bfv.sh
aws s3 cp seal_bfv_cardio.csv ${S3_URL}/${S3_FOLDER}/SEAL-BFV/
aws s3 cp fhe_parameters.txt ${S3_URL}/${S3_FOLDER}/SEAL-BFV/

# cardio BFV
./run-cardio-bfv-batched.sh
aws s3 cp seal_batched_bfv_cardio.csv ${S3_URL}/${S3_FOLDER}/SEAL-BFV-Batched/
aws s3 cp fhe_parameters.txt ${S3_URL}/${S3_FOLDER}/SEAL-BFV-Batched/

# cardio CKKS batched
./run-cardio-ckks-batched.sh
aws s3 cp seal_batched_ckks_cardio.csv ${S3_URL}/${S3_FOLDER}/SEAL-CKKS-Batched/
aws s3 cp fhe_parameters.txt ${S3_URL}/${S3_FOLDER}/SEAL-CKKS-Batched/

# cardio nn batched
./run-nn-ckks-batched.sh
aws s3 cp seal_batched_ckks_cardio.csv ${S3_URL}/${S3_FOLDER}/SEAL-CKKS-Batched/
aws s3 cp fhe_parameters_nn.txt ${S3_URL}/${S3_FOLDER}/SEAL-CKKS-Batched/