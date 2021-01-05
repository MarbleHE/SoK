FROM marblehe/base_palisade:latest

# copy eval program into container
COPY source /root/eval

WORKDIR /root/eval
RUN chmod +x docker-entrypoint.sh

# start eval program execution
ENTRYPOINT ["/root/eval/docker-entrypoint.sh"]
