#!/bin/bash

#
#    (C) Copyright 2017 CEA LIST. All Rights Reserved.
#    Contributor(s): Cingulata team (formerly Armadillo team)
#
#    This software is governed by the CeCILL-C license under French law and
#    abiding by the rules of distribution of free software.  You can  use,
#    modify and/ or redistribute the software under the terms of the CeCILL-C
#    license as circulated by CEA, CNRS and INRIA at the following URL
#    "http://www.cecill.info".
#
#    As a counterpart to the access to the source code and  rights to copy,
#    modify and redistribute granted by the license, users are provided only
#    with a limited warranty  and the software's author,  the holder of the
#    economic rights,  and the successive licensors  have only  limited
#    liability.
#
#    The fact that you are presently reading this means that you have had
#    knowledge of the CeCILL-C license and that you accept its terms.
#
#

OPT_OUTPUT_FILENAME=cingulata_cardio_optimized.csv
UNOPT_OUTPUT_FILENAME=cingulata_cardio_unoptimized.csv

UNOPTIMIZED_CIRCUIT=bfv-cardio.blif
OPTIMIZED_CIRCUIT=bfv-cardio-opt.blif

APPS_DIR=../../build_bfv/apps

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

while [[ $RUN -le 2 ]]
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
  KS=(241 210 225 219 92 43 197)

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

  # echo "FHE execution" of ABC-optimized circuit
  $APPS_DIR/dyn_omp $OPTIMIZED_CIRCUIT --threads $NR_THREADS
  FHE_EXEC_OPT_T=$( get_timestamp_ms )
  echo -n $((${FHE_EXEC_OPT_T}-${END_INPUT_ENCRYPTION_T}))"," >> $OPT_OUTPUT_FILENAME

  # echo "FHE execution" with unoptimized circuit
  $APPS_DIR/dyn_omp $UNOPTIMIZED_CIRCUIT --threads $NR_THREADS
  FHE_EXEC_UNOPT_T=$( get_timestamp_ms )
  echo -n $((${FHE_EXEC_UNOPT_T}-${FHE_EXEC_OPT_T}))"," >> $UNOPT_OUTPUT_FILENAME

  echo -ne "Decrypted result: "
  OUT_FILES=`ls -v output/*`
  $APPS_DIR/helper --from-bin --bit-cnt 8 `$APPS_DIR/decrypt  $OUT_FILES`
  DECRYPT_T=$( get_timestamp_ms )
  write_to_files $((${DECRYPT_T}-${FHE_EXEC_UNOPT_T}))"\n"
done
