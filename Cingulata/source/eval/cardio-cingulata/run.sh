#!/bin/bash

OPT_OUTPUT_FILENAME=cingulata_cardio_optimized.csv
UNOPT_OUTPUT_FILENAME=cingulata_cardio_unoptimized.csv

UNOPTIMIZED_CIRCUIT=bfv-cardio.blif
OPTIMIZED_CIRCUIT=bfv-cardio-opt.blif

APPS_DIR=/cingu/build_bfv/apps

get_timestamp_ms() {
  echo $(date +%s%3N)
}

write_to_files() {
  for item in "$@"; do
    echo -ne $item | tee -a $OPT_OUTPUT_FILENAME $UNOPT_OUTPUT_FILENAME >/dev/null
  done
}

echo "t_keygen,t_input_encryption,t_computation,t_decryption" > $OPT_OUTPUT_FILENAME
echo "t_keygen,t_input_encryption,t_computation,t_decryption" > $UNOPT_OUTPUT_FILENAME

RUN=1

while (( $RUN <= $NUM_RUNS ))
do
  RUN=$(( $RUN + 1))
  rm -rf input output
  mkdir -p input output

  START_T=$( get_timestamp_ms )

  # Generate keys
  # echo "FHE key generation"
  $APPS_DIR/generate_keys
  END_KEYGEN_T=$( get_timestamp_ms )
  write_to_files $((${END_KEYGEN_T}-${START_T}))","

  # echo "Input formatting & encryption"
  NR_THREADS=1

  # encrypt the inputs
  $APPS_DIR/encrypt --threads $NR_THREADS `$APPS_DIR/helper --bit-cnt 5 --prefix "input/i:flags_" 15`
  $APPS_DIR/encrypt --threads $NR_THREADS `$APPS_DIR/helper --bit-cnt 8 --prefix "input/i:age_" 55`
  $APPS_DIR/encrypt --threads $NR_THREADS `$APPS_DIR/helper --bit-cnt 8 --prefix "input/i:hdl_" 50`
  $APPS_DIR/encrypt --threads $NR_THREADS `$APPS_DIR/helper --bit-cnt 8 --prefix "input/i:height_" 80`
  $APPS_DIR/encrypt --threads $NR_THREADS `$APPS_DIR/helper --bit-cnt 8 --prefix "input/i:weight_" 80`
  $APPS_DIR/encrypt --threads $NR_THREADS `$APPS_DIR/helper --bit-cnt 8 --prefix "input/i:physical_act_" 45`
  $APPS_DIR/encrypt --threads $NR_THREADS `$APPS_DIR/helper --bit-cnt 8 --prefix "input/i:drinking_" 4`
  END_INPUT_ENCRYPTION_T=$( get_timestamp_ms )
  write_to_files $((${END_INPUT_ENCRYPTION_T}-${END_KEYGEN_T}))","

  # echo "FHE execution of ABC-optimized circuit"
  $APPS_DIR/dyn_omp $OPTIMIZED_CIRCUIT --threads $NR_THREADS
  FHE_EXEC_OPT_T=$( get_timestamp_ms )
  echo -n $((${FHE_EXEC_OPT_T}-${END_INPUT_ENCRYPTION_T}))"," >> $OPT_OUTPUT_FILENAME

  # echo "FHE execution of unoptimized circuit"
  $APPS_DIR/dyn_omp $UNOPTIMIZED_CIRCUIT --threads $NR_THREADS
  FHE_EXEC_UNOPT_T=$( get_timestamp_ms )
  echo -n $((${FHE_EXEC_UNOPT_T}-${FHE_EXEC_OPT_T}))"," >> $UNOPT_OUTPUT_FILENAME

  echo -ne "Decrypted result: "
  OUT_FILES=`ls -v output/*`
  $APPS_DIR/helper --from-bin --bit-cnt 8 `$APPS_DIR/decrypt  $OUT_FILES`
  DECRYPT_T=$( get_timestamp_ms )
  write_to_files $((${DECRYPT_T}-${FHE_EXEC_UNOPT_T}))"\n"
done

# Write FHE parameters into file for S3 upload
echo "== FHE parameters ====" > fhe_parameters.txt
echo "n:" $(xmlstarlet sel -t -v '/fhe_params/extra/n' < fhe_params.xml) >> fhe_parameters.txt
echo "q:" "$(xmlstarlet sel -t -v '/fhe_params/extra/q_bitsize_SEAL_BFV' < fhe_params.xml)" "($(xmlstarlet sel -t -v '/fhe_params/ciphertext/coeff_modulo_log2' < fhe_params.xml) bit)" >> fhe_parameters.txt
echo "T:" "$(xmlstarlet sel -t -v '/fhe_params/plaintext/coeff_modulo' < fhe_params.xml)" >> fhe_parameters.txt
