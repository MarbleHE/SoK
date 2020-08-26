FROM marblehe/base_cingulata

# copy eval program into container
COPY source/eval /cingu/eval

# copy entrypoint script and make it executable
COPY source/docker-entrypoint.sh /docker-entrypoint.sh
RUN chmod +x /docker-entrypoint.sh

# build the benchmark
WORKDIR /cingu

# execute the benchmark and upload benchmark results to S3
ENTRYPOINT ["/docker-entrypoint.sh"]
