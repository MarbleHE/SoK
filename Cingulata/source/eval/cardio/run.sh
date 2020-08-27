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

echo "t_keygen,t_input_encryption,t_computation,t_decryption" > $OPT_OUTPUT_FILENAME
echo "t_keygen,t_input_encryption,t_computation,t_decryption" > $UNOPT_OUTPUT_FILENAME

RUN=1

# the base circuit without any optimizations
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
  echo -n $((${END_KEYGEN_T}-${START_T}))"," >> $UNOPT_OUTPUT_FILENAME

  # echo "Input formatting & encryption"
  NR_THREADS=1
  
  # encrypt the inputs
  INPUT_NAME=(i_2 i_3 i_4 i_5 i_6 i_7 i_8 i_9 i_10 i_11 i_12 i_13 i_14 i_15 i_16 i_17 i_18 i_19 i_20 i_21 i_22 i_23 i_24 i_25 i_26 i_27 i_28 i_29 i_30 i_31 i_32 i_33 i_34 i_35 i_36 i_37 i_38 i_39 i_40 i_41 i_42 i_43 i_44 i_45 i_46 i_47 i_48 i_49 i_50 i_51 i_52 i_53 i_54 i_55 i_56 i_57 i_58 i_59 i_60 i_61 i_62 i_63 i_64 i_65 i_66 i_67 i_68 i_69 i_70 i_71 i_72 i_73 i_74 i_75 i_76 i_77 i_78 i_79 i_80 i_81 i_82 i_83 i_84 i_85 i_86 i_87 i_88 i_89 i_90 i_91 i_92 i_93 i_94 i_95 i_96 i_97 i_98 i_99 i_100 i_101 i_102 i_103 i_104 i_105 i_106 i_107 i_108 i_109 i_110 i_111 i_112 i_113)
  INPUT_VALUE=(0 0 0 1 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)

  # Encrypt client data
  for (( i = 0; i < ${#INPUT_NAME[@]}; i++ )); do
    $APPS_DIR/encrypt --threads $NR_THREADS `$APPS_DIR/helper --bit-cnt 1 --prefix "input/${INPUT_NAME[i]}" ${INPUT_VALUE[i]}`
  done
  # the helper tools creates a file in format <prefix><bit-position><suffix>
  # as we do not need the bit position (always 0), we need to remove it here from the filename
  (cd input && rename 's/....$//' * && for filename in *; do mv "$filename" "$filename.ct"; done)

  END_INPUT_ENCRYPTION_T=$( get_timestamp_ms )
  echo -n $((${END_INPUT_ENCRYPTION_T}-${END_KEYGEN_T}))"," >> $UNOPT_OUTPUT_FILENAME

  # echo "FHE execution" of unoptimized circuit
  $APPS_DIR/dyn_omp $UNOPTIMIZED_CIRCUIT --threads $NR_THREADS
  FHE_EXEC_UNOPT_T=$( get_timestamp_ms )
  echo -n $((${FHE_EXEC_UNOPT_T}-${END_INPUT_ENCRYPTION_T}))"," >> $UNOPT_OUTPUT_FILENAME

  echo -ne "Decrypted result: "
  OUT_FILES=`ls -v output/*`
  $APPS_DIR/helper --from-bin --bit-cnt 8 `$APPS_DIR/decrypt  $OUT_FILES`
  DECRYPT_T=$( get_timestamp_ms )
  echo -n $((${DECRYPT_T}-${FHE_EXEC_UNOPT_T}))"\n" >> $UNOPT_OUTPUT_FILENAME
done

# reset the run counter
RUN=1

# the Cingulata circuit optimized by ABC
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
  echo -n $((${END_KEYGEN_T}-${START_T}))"," >> $OPT_OUTPUT_FILENAME

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
  echo -n $((${END_INPUT_ENCRYPTION_T}-${END_KEYGEN_T}))"," >> $OPT_OUTPUT_FILENAME

  # echo "FHE execution" of ABC-optimized circuit
  $APPS_DIR/dyn_omp $OPTIMIZED_CIRCUIT --threads $NR_THREADS
  FHE_EXEC_OPT_T=$( get_timestamp_ms )
  echo -n $((${FHE_EXEC_OPT_T}-${END_INPUT_ENCRYPTION_T}))"," >> $OPT_OUTPUT_FILENAME

  echo -ne "Decrypted result: "
  OUT_FILES=`ls -v output/*`
  $APPS_DIR/helper --from-bin --bit-cnt 8 `$APPS_DIR/decrypt  $OUT_FILES`
  DECRYPT_T=$( get_timestamp_ms )
  echo -n $((${DECRYPT_T}-${FHE_EXEC_OPT_T}))"\n" >> $OPT_OUTPUT_FILENAME
done

# Write FHE parameters into file for S3 upload
echo "== FHE parameters ====" > fhe_parameters.txt
echo "n:" $(xmlstarlet sel -t -v '/fhe_params/extra/n' < fhe_params.xml) >> fhe_parameters.txt
echo "q:" "$(xmlstarlet sel -t -v '/fhe_params/extra/q_bitsize_SEAL_BFV' < fhe_params.xml)" "($(xmlstarlet sel -t -v '/fhe_params/ciphertext/coeff_modulo_log2' < fhe_params.xml) bit)" >> fhe_parameters.txt
echo "T:" "$(xmlstarlet sel -t -v '/fhe_params/plaintext/coeff_modulo' < fhe_params.xml)" >> fhe_parameters.txt
