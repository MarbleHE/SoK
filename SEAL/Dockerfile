FROM marblehe/base_seal

# copy eval program into container
# COPY source /root/eval
WORKDIR /root/eval

# build the benchmark
# RUN cd /root/eval && mkdir build && cd /root/eval/build && cmake .. && make
# WORKDIR /root/eval/build

# execute the benchmark and upload benchmark results to S3
# CMD ["sh", "-c", "./run.sh; aws s3 cp seal_cardio.csv ${S3_URL}/${S3_FOLDER}/SEAL/"]

CMD ["/bin/bash"]
