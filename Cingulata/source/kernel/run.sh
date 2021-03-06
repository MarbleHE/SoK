#!/bin/bash

APPS_DIR=/cingu/build_bfv/apps
IMAGE_SIZE=8

CIRCUIT=bfv-kernel-opt.blif
OUTPUT_FILENAME=cingulata_kernel.csv

get_timestamp_ms() {
  echo $(date +%s%3N)
}

echo "t_keygen,t_input_encryption,t_computation,t_decryption" > $OUTPUT_FILENAME

RUN=1
while (( $RUN <= $NUM_RUNS ))
do
    RUN=$(( $RUN + 1))
    mkdir -p input output
    rm -f input/*.ct output/*.ct

    # Generate keys
    START_KEYGEN_T=$( get_timestamp_ms )
    echo "FHE key generation"
    $APPS_DIR/generate_keys
    END_KEYGEN_T=$( get_timestamp_ms )
    echo -ne $((${END_KEYGEN_T}-${START_KEYGEN_T}))"," >> $OUTPUT_FILENAME

    START_INPUT_ENC_T=$( get_timestamp_ms )
    echo "Input encryption"
    NR_THREADS=$(nproc)
    # the input is, for example for IMAGE_SIZE=8 (i.e., 8x8=64 pixel):
    # 0 1 2 3 4 5 6 7 
    # 0 1 2 3 4 5 6 7 
    # ... repeat 5x ...
    # 0 1 2 3 4 5 6 7
    $APPS_DIR/encrypt -v `$APPS_DIR/helper --bit-cnt 8 $(for (( i=0; i<${IMAGE_SIZE}; i++ )); do seq 0 $(( ${IMAGE_SIZE}-1 )); done | tr '\n' ' ')` --threads $NR_THREADS
    END_INPUT_ENC_T=$( get_timestamp_ms )
    echo -ne $((${END_INPUT_ENC_T}-${START_INPUT_ENC_T}))"," >> $OUTPUT_FILENAME

    START_EXECUTION_T=$( get_timestamp_ms )
    echo "Homomorphic execution..."
    time $APPS_DIR/dyn_omp $CIRCUIT --threads $NR_THREADS # -v
    END_EXECUTION_T=$( get_timestamp_ms )
    echo -ne $((${END_EXECUTION_T}-${START_EXECUTION_T}))"," >> $OUTPUT_FILENAME

    START_DECRYPTION_T=$( get_timestamp_ms )
    echo "Output decryption"
    OUT_FILES=`ls -v output/*`
    $APPS_DIR/helper --from-bin --bit-cnt 8 `$APPS_DIR/decrypt --threads $NR_THREADS  $OUT_FILES `
    END_DECRYPTION_T=$( get_timestamp_ms )
    echo -ne $((${END_DECRYPTION_T}-${START_DECRYPTION_T}))"\n" >> $OUTPUT_FILENAME

    ls -l --human-readable output
done

# Write FHE parameters into file for S3 upload
PARAMS_FILE=fhe_parameters_kernel.txt
echo "== FHE parameters ====" > ${PARAMS_FILE}
echo "n:" $(xmlstarlet sel -t -v '/fhe_params/extra/n' < fhe_params.xml) >> ${PARAMS_FILE}
echo "q:" "$(xmlstarlet sel -t -v '/fhe_params/extra/q_bitsize_SEAL_BFV' < fhe_params.xml)" "($(xmlstarlet sel -t -v '/fhe_params/ciphertext/coeff_modulo_log2' < fhe_params.xml) bit)" >> ${PARAMS_FILE}
echo "T:" "$(xmlstarlet sel -t -v '/fhe_params/plaintext/coeff_modulo' < fhe_params.xml)" >> ${PARAMS_FILE}
