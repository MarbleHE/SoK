#!/bin/bash

upload_file() {
    TARGET_DIR=$1
    shift
    for item in "$@"; do
        aws s3 cp $item ${S3_URL}/${S3_FOLDER}/${TARGET_DIR}/
    done  
}

echo "Running chi-squared..."
cd /cingu/eval/chi-squared \
    && ./run.sh \
    && upload_file Cingulata cingulata_chi_squared_unoptimized.csv fhe_parameters_chi_squared.txt

echo "Running cardio-cingulata..."
cd /cingu/eval/cardio-cingulata \
    && ./run.sh \
    && upload_file Cingulata-OPT cingulata_cardio_optimized.csv fhe_parameters.txt \
    && upload_file Cingulata-Baseline cingulata_cardio_unoptimized.csv fhe_parameters.txt

echo "Running cardio-lobster-baseline..."
cd /cingu/eval/cardio-lobster-baseline \
    && ./run.sh \
    && upload_file Lobster-Baseline cardio_lobster_baseline_unoptimized.csv fhe_parameters.txt \
    && upload_file Lobster-Baseline-OPT cardio_lobster_baseline_optimized.csv fhe_parameters.txt

echo "Running cardio-lobster..."
cd /cingu/eval/cardio-lobster \
    && ./run.sh \
    && upload_file Lobster cingulata_cardio_lobster.csv fhe_parameters.txt

echo "Running cardio-lobster-optimal-params..."
cd /cingu/eval/cardio-lobster-optimal-params \
    && ./run.sh \
    && upload_file Lobster-OPT-PARAMS cardio_lobster_optimal.csv fhe_parameters.txt

echo "Running cardio-multistart..."
cd /cingu/eval/cardio-multistart \
    && ./run.sh \
    && upload_file MultiStart cardio_multistart.csv fhe_parameters.txt

echo "Running cardio-multistart-optimal-params..."
cd /cingu/eval/cardio-multistart-optimal-params \
    && ./run.sh \
    && upload_file MultiStart-OPT-PARAMS cingulata_cardio_multistart_optimal.csv fhe_parameters.txt
