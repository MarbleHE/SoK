FROM marblehe/base_sealion

# == NOTE =========================================
# This usually happens in the base image but as SEALion is non-public, we need to install it here (i.e., before running benchmarks) so that it is not pushed to Dockerhub.

# get SEALion from private repository
COPY id_rsa /root/.ssh/id_rsa
RUN chmod 600 /root/.ssh/id_rsa; ssh-keyscan github.com >> /root/.ssh/known_hosts
RUN ssh-agent bash -c 'ssh-add /root/.ssh/id_rsa; git clone --recursive git@github.com:MarbleHE/sealion.git' 

# install SEALion python package
WORKDIR /root/sealion
RUN CFLAGS=-O3 python3 setup.py install

# =========================================

# copy eval program into container
COPY source /root/eval
WORKDIR /root/eval

# execute the benchmark and upload benchmark results to S3
CMD ["sh", "-c", "python3 mlp.py > result_benchmark_sealion.txt; aws s3 cp result_benchmark_sealion.txt s3://sok-repository-eval-benchmarks"]



