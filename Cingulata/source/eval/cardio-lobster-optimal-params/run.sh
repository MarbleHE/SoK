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

OPT_OUTPUT_FILENAME=cingulata_cardio_lobster_optimal.csv
OPTIMIZED_CIRCUIT=cardio_lobster.blif
APPS_DIR=../../build_bfv/apps

get_timestamp_ms() {
  echo $(date +%s%3N)
}

write_to_files() {
  for item in "$@"; do
    echo -ne $item | tee -a $OPT_OUTPUT_FILENAME >/dev/null
  done
}

echo "t_keygen,t_input_encryption,t_computation,t_decryption" > $OPT_OUTPUT_FILENAME

RUN=1

while [[ $RUN -le 10 ]]
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
  # These are the same inputs as used by Lobster.
  INPUT_NAME=(i:10 i:100 i:101 i:102 i:103 i:104 i:105 i:106 i:107 i:108 i:109 i:11 i:110 i:111 i:112 i:113 i:12 i:13 i:14 i:15 i:16 i:17 i:18 i:19 i:20 i:21 i:22 i:23 i:24 i:25 i:26 i:27 i:28 i:29 i:30 i:31 i:32 i:33 i:34 i:35 i:36 i:37 i:38 i:39 i:40 i:41 i:42 i:43 i:44 i:45 i:46 i:47 i:48 i:49 i:5 i:50 i:51 i:52 i:53 i:54 i:55 i:56 i:57 i:6 i:61 i:62 i:63 i:64 i:65 i:66 i:67 i:68 i:69 i:7 i:70 i:71 i:72 i:73 i:74 i:75 i:76 i:77 i:78 i:79 i:8 i:80 i:81 i:82 i:83 i:84 i:85 i:86 i:87 i:88 i:89 i:9 i:90 i:91 i:92 i:93 i:94 i:95 i:96 i:97 i:98 i:99)
  INPUT_VALUE=(0 0 0 1 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)

  # Encrypt client data
  for (( i = 0; i < ${#INPUT_NAME[@]}; i++ )); do
    $APPS_DIR/encrypt --threads $NR_THREADS `$APPS_DIR/helper --bit-cnt 1 --prefix "input/${INPUT_NAME[i]}" ${INPUT_VALUE[i]}`
  done
  # the helper tools creates a file in format <prefix><bit-position><suffix>
  # as we do not need the bit position (always 0), we need to remove it here from the filename
  (cd input && rename 's/....$//' * && for filename in *; do mv "$filename" "$filename.ct"; done)

  END_ENCRYPTION_T=$( get_timestamp_ms )
  write_to_files $((${END_ENCRYPTION_T}-${END_KEYGEN_T}))","

  # echo "FHE execution" of optimized circuit
  $APPS_DIR/dyn_omp $OPTIMIZED_CIRCUIT --threads $NR_THREADS
  FHE_EXEC_OPT_T=$( get_timestamp_ms )
  write_to_files $((${FHE_EXEC_OPT_T}-${END_ENCRYPTION_T}))","

  echo -ne "Decrypted result: "
  OUT_FILES=`ls -v output/*`
  $APPS_DIR/helper --from-bin --bit-cnt 8 `$APPS_DIR/decrypt  $OUT_FILES`
  DECRYPT_T=$( get_timestamp_ms )
  write_to_files $((${DECRYPT_T}-${FHE_EXEC_OPT_T}))"\n"
done
