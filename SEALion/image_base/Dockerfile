FROM ubuntu:bionic

RUN apt-get update && apt-get install -y python3 python3-dev python3-pip python3-setuptools gcc-6 g++-6 git openssh-client

RUN mkdir -p /root/.ssh

WORKDIR /root

# Run example: 
#   python3 examples/tutorials/mlp.py
CMD ["/bin/bash"]

