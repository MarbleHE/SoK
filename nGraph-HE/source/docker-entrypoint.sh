#!/bin/bash

upload_files() {
    TARGET_DIR=$1
    shift
    for item in "$@"; do
        aws s3 cp $item ${S3_URL}/${S3_FOLDER}/${TARGET_DIR}/
    done
}

# MLP with x^2
source /home/he-transformer/build/external/venv-tf-py3/bin/activate
export OUTPUT_FILENAME=ngraph-he-mlp-squared_nn.csv
python3 nn-mlp-squared/mnist.py
upload_files nGraph-HE-MLP-squared ${OUTPUT_FILENAME}

# MLP with a*x^2 + bx
source /home/he-transformer/build/external/venv-tf-py3/bin/activate
export OUTPUT_FILENAME=ngraph-he-mlp-learned_nn.csv
python3 nn-mlp-learned/mnist.py
upload_files nGraph-HE-MLP-learned ${OUTPUT_FILENAME}

# Cryptonets with x^2
source /home/he-transformer/build/external/venv-tf-py3/bin/activate
export OUTPUT_FILENAME=ngraph-he-cryptonets-squared_nn.csv
python3 nn-cryptonets-squared/mnist.py
upload_files nGraph-HE-Cryptonets-squared ${OUTPUT_FILENAME}

# Cryptonets with a*x^2 + bx
# Did not work?
#source /home/he-transformer/build/external/venv-tf-py3/bin/activate
#export OUTPUT_FILENAME=ngraph-he-cryponets-learned_nn.csv
#python3 nn-cryptonets-learned/mnist.py
#upload_files nGraph-HE-Cryptonets-learned ${OUTPUT_FILENAME}