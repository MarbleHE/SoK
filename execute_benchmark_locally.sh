#!/bin/bash

read -r -d '' USAGE << EOM
Usage: ./execute_benchmark_locally.sh <toolname>
    where <toolname> = [alchemy|cingulata|lobster|seal|sealion|tfhe|ngraph] ...

Follow environment variables are required for the S3 upload:
    - AWS_ACCESS_KEY_ID
    - AWS_SECRET_ACCESS_KEY
EOM


if [ $# == 0 ] ; then
    echo -e "$USAGE"
    exit 1;
fi

# this ensures that all tools write their results into the same folder
S3_FOLDER_NAME=$(date +"%Y%m%d_%H%M%S")

for TOOLNAME in "$@"
do
  # toolname is an abbreviation for the tool that is compatible with the Docker
  # tag naming conventions

  case "$TOOLNAME" in
    alchemy)   
      tooldir=ALCHEMY
      ;;
    cingulata)
      tooldir=Cingulata
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
    e3)
      tooldir=E3
      ;;
    *) 
      echo "Given TOOLNAME '${TOOLNAME}' not found!"
      exit 1;
  esac

  TAG_NAME=eval_${TOOLNAME}

  cd ${tooldir} || (echo "Could not find ${tooldir} directory! Needs to be executed from SoK root directory."; exit 1)

  echo "Building eval image for ${tooldir}..."
  docker build -t ${TAG_NAME} . && \
  echo "Running benchmark in eval docker image..." && \
  docker run -e S3_URL=s3://sok-repository-eval-benchmarks \
      -e S3_FOLDER=${S3_FOLDER_NAME} \
      -e AWS_ACCESS_KEY_ID=${AWS_ACCESS_KEY_ID} \
      -e AWS_SECRET_ACCESS_KEY=${AWS_SECRET_ACCESS_KEY} \
      -e AWS_DEFAULT_REGION=us-east-2 \
      -e NUM_RUNS=1 \
      -it ${TAG_NAME}
done
