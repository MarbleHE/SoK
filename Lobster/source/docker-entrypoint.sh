#!/bin/bash

# See the Lobster repository for more details: https://github.com/ropas/PLDI2020_242_artifact_publication#data-description
# optimize circuit 'hd01' with baseline (Carpov et al.)
../baseline/main.native hd01.eqn ../baseline/baseline_cases > result_hd01_baseline.txt
aws s3 cp result_hd01_baseline.txt ${S3_URL}/${S3_FOLDER}/Lobster/

# # optimize circuit 'hd01' with machine-found aggresive optimization patterns by offline-learning (Section 4.2)
../baseline/main.native hd01.eqn ../circuit_rewriting/paper_cases/all_cases > result_hd01_lobster.txt
aws s3 cp result_hd01_lobster.txt ${S3_URL}/${S3_FOLDER}/Lobster/

# homomorphically evaluate 'hd01'
../homomorphic_evaluation/mc_parser/he_base test.eqn 1 > result_test_eval.txt
aws s3 cp result_test_eval.txt ${S3_URL}/${S3_FOLDER}/Lobster/

