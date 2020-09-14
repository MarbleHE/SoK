FROM marblehe/base_seal

# copy eval program into container
COPY source /root/eval
WORKDIR /root/eval

# build the benchmark
RUN cd /root/eval && mkdir build && cd /root/eval/build && cmake .. && make -j$(nproc)

WORKDIR /root/eval
RUN chmod +x docker-entrypoint.sh

# start eval program execution
ENTRYPOINT ["/root/eval/docker-entrypoint.sh"]
