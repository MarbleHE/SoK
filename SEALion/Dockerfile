FROM marblehe/base_sealion

# == NOTE =========================================
# This usually happens in the base image but as SEALion is non-public, we need to install it here (i.e., before running benchmarks) so that it is not pushed to Dockerhub.

# get SEALion from private repository
COPY id_sealion /root/.ssh/id_rsa
RUN chmod 600 /root/.ssh/id_rsa; ssh-keyscan github.com >> /root/.ssh/known_hosts
RUN ssh-agent bash -c 'ssh-add /root/.ssh/id_rsa; git clone --recursive git@github.com:MarbleHE/sealion.git' 

# install SEALion python package
WORKDIR /root/sealion
RUN CFLAGS=-O3 python3 setup.py install

# =========================================

# Install pandas, which is used to create the benchmarking csv
RUN pip3 install pandas

# copy eval program into container
COPY source /root/eval
WORKDIR /root/eval

# execute the benchmark and upload benchmark results to S3
CMD ["sh", "-c", "export OUTPUT_FILENAME=sealion_nn.csv; python3 mnist.py; aws s3 cp ${OUTPUT_FILENAME} ${S3_URL}/${S3_FOLDER}/SEALion/"]
