FROM marblehe/base_lobster:latest

# copy eval program into container
COPY source /root/Lobster/eval

WORKDIR /root/Lobster/eval
RUN chmod +x docker-entrypoint.sh

# start eval program execution
ENTRYPOINT ["/root/Lobster/eval/docker-entrypoint.sh"]
