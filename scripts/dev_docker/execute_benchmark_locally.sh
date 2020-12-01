#!/bin/bash

# This script takes as arugment the name of the tool in lower-case letters.
# It executes all benchmarks of the given tool and uploads the resulting files to S3.

read -r -d '' USAGE <<EOM
Usage: ./execute_benchmark_locally.sh <toolname>
    where <toolname> = [alchemy|cingulata|concrete|e3|eva|helib|lobster|palisade|seal|sealion|tfhe|ngraph]

Following environment variables are required for the S3 upload:
    - AWS_ACCESS_KEY_ID
    - AWS_SECRET_ACCESS_KEY
EOM

if [[ -z "${AWS_ACCESS_KEY_ID}" ]]; then
  echo -e "NOTE: The S3 upload of the results won't work because the env variable AWS_ACCESS_KEY_ID was not found.\nRun 'export AWS_ACCESS_KEY_ID=<your key>' before running this script."
fi

if [[ -z "${AWS_SECRET_ACCESS_KEY}" ]]; then
  echo -e "NOTE: The S3 upload of the results won't work because the env variable AWS_SECRET_ACCESS_KEY was not found.\nRun 'export AWS_SECRET_ACCESS_KEY=<your key>' before running this script."
fi

if [ $# == 0 ]; then
  echo -e "$USAGE"
  exit 1
fi

# this ensures that all tools write their results into the same folder
S3_FOLDER_NAME=$(date +"%Y%m%d_%H%M%S")

CUR_DIR=$(pwd)

for TOOLNAME in "$@"; do
  # toolname is an abbreviation for the tool that is compatible with the Docker
  # tag naming conventions

  case "$TOOLNAME" in
  alchemy)
    tooldir=ALCHEMY
    ;;
  cingulata)
    tooldir=Cingulata
    ;;
  concrete)
    tooldir=Concrete
    ;;
  e3)
    tooldir=E3
    ;;
  eva)
    tooldir=EVA
    ;;
  helib)
    tooldir=HElib
    ;;
  lobster)
    tooldir=Lobster
    ;;
  seal)
    tooldir=SEAL
    ;;
  sealion)
    tooldir=SEALion
    ;;
  tfhe)
    tooldir=TFHE
    ;;
  ngraph)
    tooldir=nGraph-HE
    ;;
  palisade)
    tooldir=PALISADE
    ;;
  *)
    echo "Given TOOLNAME '${TOOLNAME}' not found!"
    exit 1
    ;;
  esac

  (cd ${tooldir} &&
    echo "Building eval image for ${tooldir} and running benchmark programs ..." &&
    docker run -d -e S3_URL=s3://sok-repository-eval-benchmarks \
      -e S3_FOLDER=${S3_FOLDER_NAME} \
      -e AWS_ACCESS_KEY_ID=${AWS_ACCESS_KEY_ID} \
      -e AWS_SECRET_ACCESS_KEY=${AWS_SECRET_ACCESS_KEY} \
      -e AWS_DEFAULT_REGION=us-east-2 \
      -e NUM_RUNS=1 \
      -it $(docker build -q .))
done
