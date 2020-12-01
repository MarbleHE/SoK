FROM ubuntu:20.04

###### install all required packages
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update; \ 
    apt-get -y install git wget awscli build-essential make gcc-8 clang-tools-9 cmake libmsgsl-dev zlib1g-dev gcc-multilib fftw3 libfftw3-dev libfftw3-doc curl libssl-dev yasm
 
###### build E3
RUN git clone https://github.com/momalab/e3.git && \
    cd e3 && git checkout 64a704c398c589de817bab9fd6fc8a4ac0439575 && \
    cd src 
#&& make -j $(nproc)

###### download and build target libraries
WORKDIR /e3/3p
RUN export MAKEFLAGS=-j`nproc` && make SEAL && make TFHE

# The tutorial described in
#   https://github.com/momalab/e3/blob/master/tutorials/basic/README.md
# could be executed successfully.

CMD ["/bin/bash"]
