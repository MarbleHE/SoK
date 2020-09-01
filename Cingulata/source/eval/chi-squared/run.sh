#!/bin/bash

UNOPT_OUTPUT_FILENAME=cingulata_chi-squared_unoptimized.csv

UNOPT_CIRCUIT=bfv-chi-squared.blif

APPS_DIR=/cingu/build_bfv/apps

get_timestamp_ms() {
  echo $(date +%s%3N)
}

write_to_files() {
  for item in "$@"; do
    echo -ne $item | tee -a $UNOPT_OUTPUT_FILENAME >/dev/null
  done
}

write_to_files "t_keygen,t_input_encryption,t_computation,t_decryption\n"

RUN=1

while (($RUN <= $NUM_RUNS)); do
  RUN=$(($RUN + 1))
  rm -rf input output
  mkdir -p input output

  START_T=$(get_timestamp_ms)

  # Generate keys
  echo "FHE key generation"
  $APPS_DIR/generate_keys
  END_KEYGEN_T=$(get_timestamp_ms)
  write_to_files $((${END_KEYGEN_T} - ${START_T}))","

  # encrypt the inputs
  echo "Input encryption"
  NR_THREADS=$(nproc)
  $APPS_DIR/encrypt --threads $NR_THREADS $($APPS_DIR/helper --bit-cnt 8 --prefix "input/i:n0_" 2)
  $APPS_DIR/encrypt --threads $NR_THREADS $($APPS_DIR/helper --bit-cnt 8 --prefix "input/i:n1_" 7)
  $APPS_DIR/encrypt --threads $NR_THREADS $($APPS_DIR/helper --bit-cnt 8 --prefix "input/i:n2_" 9)
  END_INPUT_ENCRYPTION_T=$(get_timestamp_ms)
  write_to_files $((${END_INPUT_ENCRYPTION_T} - ${END_KEYGEN_T}))","

  echo "FHE execution of unoptimized circuit"
  $APPS_DIR/dyn_omp $UNOPT_CIRCUIT --threads $NR_THREADS
  FHE_EXEC_UNOPT_T=$(get_timestamp_ms)
  write_to_files $((${FHE_EXEC_UNOPT_T} - ${END_INPUT_ENCRYPTION_T}))","

  echo -ne "Decrypted result: "
  OUT_FILES=$(ls -v output/*)
  $APPS_DIR/helper --from-bin --bit-cnt 16 $($APPS_DIR/decrypt --threads $NR_THREADS $OUT_FILES)
  DECRYPT_T=$(get_timestamp_ms)
  write_to_files $((${DECRYPT_T} - ${FHE_EXEC_UNOPT_T}))"\n"
done

# Write FHE parameters into file for S3 upload
INPUT_FILE=fhe_params.xml
OUTPUT_FILE=fhe_parameters_chi_squared.txt
echo "== FHE parameters ====" >${OUTPUT_FILE}
echo "n:" $(xmlstarlet sel -t -v '/fhe_params/extra/n' <${INPUT_FILE}) >>${OUTPUT_FILE}
echo "q:" "$(xmlstarlet sel -t -v '/fhe_params/extra/q_bitsize_SEAL_BFV' <${INPUT_FILE})" "($(xmlstarlet sel -t -v '/fhe_params/ciphertext/coeff_modulo_log2' <${INPUT_FILE}) bit)" >>${OUTPUT_FILE}
echo "T:" "$(xmlstarlet sel -t -v '/fhe_params/plaintext/coeff_modulo' <${INPUT_FILE})" >>${OUTPUT_FILE}
