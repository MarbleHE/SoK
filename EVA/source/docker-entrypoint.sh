#!/bin/bash

upload_files() {
    TARGET_DIR=$1
    shift
    for item in "$@"; do
        aws s3 cp $item ${S3_URL}/${S3_FOLDER}/${TARGET_DIR}/
    done
}

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
