FROM marblehe/base_ngraph-he

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y awscli

# Install pandas, which is used to create the benchmarking csv
RUN ["/bin/bash", "-c", "source /home/he-transformer/build/external/venv-tf-py3/bin/activate && pip3 install pandas"]

# copy eval program into container
COPY source /root/eval
WORKDIR /root/eval
RUN chmod +x /root/eval/docker-entrypoint.sh

# start eval program execution
ENTRYPOINT ["/root/eval/docker-entrypoint.sh"]
