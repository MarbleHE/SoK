FROM marblehe/base_ngraph-he

# Install pandas, which is used to create the benchmarking csv
RUN ["/bin/bash", "-c", "source /home/he-transformer/build/external/venv-tf-py3/bin/activate && pip3 install pandas"]

# copy eval program into container
COPY source /root/eval
WORKDIR /root/eval

# execute the benchmark and upload benchmark results to S3
CMD ["/bin/bash", "-c", "source /home/he-transformer/build/external/venv-tf-py3/bin/activate; python3 mnist.py; aws s3 cp sealion_nn.csv ${S3_URL}/${S3_FOLDER}/nGraph-HE/"]