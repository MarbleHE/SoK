FROM marblehe/base_cingulata

# copy eval program into container
COPY source/ /cingu/eval

# make entrypoint script executable
WORKDIR /cingu/eval
RUN chmod +x docker-entrypoint.sh

# execute the benchmark and upload benchmark results to S3
ENTRYPOINT ["/cingu/eval/docker-entrypoint.sh"]
