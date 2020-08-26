#!/bin/bash

# run cardio in Cingulata for both the ABC-optimized circuit and the unoptimized
# circuit
cd /cingu/eval/cardio
./run.sh
aws s3 cp cingulata_cardio_optimized.csv ${S3_URL}/${S3_FOLDER}/Cingulata/
aws s3 cp cingulata_cardio_unoptimized.csv ${S3_URL}/${S3_FOLDER}/Cingulata-UNOPT/

# run cardio in Cingulata with circuit optimized by Lobster and same parameters as Cingulata uses
cd /cingu/eval/cardio-lobster
./run.sh
aws s3 cp cingulata_cardio_lobster.csv ${S3_URL}/${S3_FOLDER}/Lobster/

# run cardio in Cingulata with circuit optimized by Lobster and parameters determined by CinguParam
cd /cingu/eval/cardio-lobster-optimal-params
./run.sh
aws s3 cp cingulata_cardio_lobster_optimal.csv ${S3_URL}/${S3_FOLDER}/Lobster-OPT-PARAMS/

# run cardio in Cingulata with circuit optimized by Lobster and parameters determined by CinguParam
cd /cingu/eval/cardio-multistart
./run.sh
aws s3 cp cingulata_cardio_multistart.csv ${S3_URL}/${S3_FOLDER}/Cingulata-MultiStart/

# run cardio in Cingulata with circuit optimized by Lobster and parameters determined by CinguParam
cd /cingu/eval/cardio-multistart-optimal-params
./run.sh
aws s3 cp cingulata_cardio_multistart_optimal.csv ${S3_URL}/${S3_FOLDER}/Cingulata-MultiStart-OPT-PARAMS/

