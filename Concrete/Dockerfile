FROM marblehe/base_concrete

# copy eval program into container
RUN mkdir /eval
COPY source/ /eval

# copy entrypoint script and make it executable
COPY source/docker-entrypoint.sh /docker-entrypoint.sh
RUN chmod +x /docker-entrypoint.sh

# build the benchmark
WORKDIR /eval

# execute the benchmark and upload benchmark results to S3
ENTRYPOINT ["/docker-entrypoint.sh"]
