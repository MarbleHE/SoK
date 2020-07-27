FROM marblehe/base_cingulata

# copy eval program into container
COPY source /root/eval

# build the benchmark
WORKDIR /root/eval
RUN mkdir build
WORKDIR /root/eval/build
RUN cmake ..; make

# execute the benchmark and upload benchmark results to S3
CMD ["sh", "-c", "./benchmark > result_benchmark_cingulata.csv; aws s3 cp result_benchmark_cingulata.csv s3://sok-repository-eval-benchmarks"]