FROM marblehe/base_e3

# copy eval program into container
COPY source/ /e3/eval
COPY source/mak_cgt.mak /e3/src/mak_cgt.mak

# copy entrypoint script and make it executable
COPY source/docker-entrypoint.sh /docker-entrypoint.sh
RUN chmod +x /docker-entrypoint.sh

# build the benchmark
WORKDIR /e3/eval

# execute the benchmark and upload benchmark results to S3
ENTRYPOINT ["/docker-entrypoint.sh"]
