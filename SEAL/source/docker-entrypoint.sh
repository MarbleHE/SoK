#!/bin/bash

EVAL_BUILD_DIR=/root/eval/build
cd $EVAL_BUILD_DIR

# cardio BFV
./run-cardio.sh
aws s3 cp seal_bfv_cardio.csv ${S3_URL}/${S3_FOLDER}/SEAL/

# cardio CKKS batched
./run-cardio-batched.sh
aws s3 cp seal_batched_ckks_cardio.csv ${S3_URL}/${S3_FOLDER}/SEAL/
