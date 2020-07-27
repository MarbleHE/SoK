FROM ubuntu:bionic

RUN apt-get update && apt-get install -y python3 python3-dev python3-pip python3-setuptools gcc-6 g++-6 git ssh

RUN mkdir -p /root/.ssh
COPY id_rsa /root/.ssh/id_rsa
RUN chmod 600 /root/.ssh/id_rsa; ssh-keyscan github.com >> /root/.ssh/known_hosts

WORKDIR /root
RUN ssh-agent bash -c 'ssh-add /root/.ssh/id_rsa; git clone --recursive git@github.com:MarbleHE/sealion.git' 

WORKDIR /root/sealion
RUN pip3 install -U --user utils numpy
RUN CFLAGS=-O3 python3 setup.py install

# Run example: 
#   python3 examples/tutorials/mlp.py
CMD ["/bin/bash"]

