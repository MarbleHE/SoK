FROM marblehe/base_tfhe

# copy eval program into container
COPY source /root/eval

# build the benchmark
WORKDIR /root/eval
RUN mkdir build
WORKDIR /root/eval/build
RUN cmake .. && make

# execute the benchmark and upload benchmark results to S3
CMD ["sh", "-c", "./run.sh; aws s3 cp tfhe_cardio.csv ${S3_URL}/${S3_FOLDER}/TFHE/"]
