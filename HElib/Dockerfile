FROM marblehe/base_helib:latest

# copy eval program into container
COPY source /root/eval

WORKDIR /root/eval
COPY source/docker-entrypoint.sh /
RUN chmod +x /docker-entrypoint.sh

# start eval program execution
ENTRYPOINT ["/docker-entrypoint.sh"]
