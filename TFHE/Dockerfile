FROM marblehe/base_tfhe

# copy eval program into container
COPY source /root/eval
RUN chmod +x /root/eval/docker-entrypoint.sh

# build the benchmark
WORKDIR /root/eval
RUN cd /root/eval && \
    mkdir build && \
    cd /root/eval/build && \
    cmake .. && \
    make

# execute the benchmark and upload benchmark results to S3
ENTRYPOINT ["/root/eval/docker-entrypoint.sh"]
