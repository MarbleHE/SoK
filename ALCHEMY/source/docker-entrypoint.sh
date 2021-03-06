#!/bin/bash

cd /root/eval
stack setup
# executable to build (alchemy-arithmetic) is defined in stack.yaml
stack build --exec alchemy-arithmetic > alchemy_result_arithmetic.txt
aws s3 cp alchemy_result_arithmetic.txt ${S3_URL}/${S3_FOLDER}/ALCHEMY/