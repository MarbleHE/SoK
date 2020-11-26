#!/bin/bash
set -euo pipefail
shopt -s inherit_errexit

export EVAL_DIR=/eval
export MAKEFLAGS=-j$(nproc)

upload_files() {
    TARGET_DIR=$1
    shift
    for item in "$@"; do
        aws s3 cp $item ${S3_URL}/${S3_FOLDER}/${TARGET_DIR}/
    done  
}

get_timestamp_ms() {
  echo $(date +%s%3N)
}


###############
# Programs
###############

cd $EVAL_DIR/example_program \
    && cargo build \
    && cd target \
    && cd debug \
    && ./main
