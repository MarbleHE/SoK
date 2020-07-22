FROM marblehe/base_alchemy

# copy eval program into container
RUN mkdir -p /root/eval/
COPY source /root/eval

WORKDIR /root/eval
RUN chmod +x docker-entrypoint.sh

# start eval program execution
ENTRYPOINT ["/root/eval/docker-entrypoint.sh"]
