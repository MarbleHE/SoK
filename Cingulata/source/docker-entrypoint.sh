#!/bin/bash

# run cardio in Cingulata for both the ABC-optimized circuit and the unoptimized
# circuit
cd /cingu/eval/cardio
./run.sh
aws s3 cp cingulata_cardio_optimized.csv ${S3_URL}/${S3_FOLDER}/Cingulata/
aws s3 cp cingulata_cardio_unoptimized.csv ${S3_URL}/${S3_FOLDER}/Cingulata-UNOPT/
