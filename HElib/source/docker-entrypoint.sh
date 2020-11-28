#!/bin/bash

# build & execute the sample program
cd Microbenchmark \
    && mkdir build \
    && cd build \
    && cmake .. \
    && make -j$(nproc) \
    && ./microbenchmark
# aws s3 cp result_hd01_baseline.txt ${S3_URL}/${S3_FOLDER}/Lobster/
