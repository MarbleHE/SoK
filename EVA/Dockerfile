FROM marblehe/base_eva

# copy eval program into container
COPY source /root/eval

# apply the patch to EVA
RUN apt-get install -y rsync
RUN rsync -r /root/eval/eva_patch/ /EVA

# recompile eva
RUN cd EVA; \
    cmake . && \
    make -j$(nproc) && \
    python3 -m pip install -e python/ && \
    python3 -m pip install -r examples/requirements.txt

WORKDIR /root/eval
COPY source/docker-entrypoint.sh /
RUN chmod +x /docker-entrypoint.sh

# start eval program execution
ENTRYPOINT ["/docker-entrypoint.sh"]
