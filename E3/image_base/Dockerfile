FROM ubuntu:20.04

###### install all required packages
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update; \ 
    apt-get -y install git wget awscli build-essential make gcc-8 clang-tools-9 cmake libmsgsl-dev zlib1g-dev gcc-multilib fftw3 libfftw3-dev libfftw3-doc curl libssl-dev yasm
 
###### build E3
RUN git clone https://github.com/momalab/e3.git && \
    cd e3 && git checkout 4ecbe107a07101b7ed6f6362e9dde8aa91c17578 && \
    cd src 
#&& make -j $(nproc)

###### build target libraries
WORKDIR /e3/3p
RUN export MAKEFLAGS=-j`nproc` && \
    ./seal_unx.sh && \
    ./fhew_unx.sh && \
    ./heli_unx.sh && \
    ./mpir_unx.sh && \
    ./tfhe_unx.sh

# The tutorial described in
#   https://github.com/momalab/e3/blob/master/tutorials/basic/README.md
# could be executed successfully.

CMD ["/bin/bash"]
