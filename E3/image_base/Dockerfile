FROM ubuntu:20.04

###### install all required packages
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update; \ 
    apt-get -y install git wget awscli build-essential make gcc-8 clang-tools-9 cmake libmsgsl-dev zlib1g-dev libomp-dev fftw3 libfftw3-dev libfftw3-doc curl
 
###### build cmake from source (to get a new enough version for SEAL)
RUN wget https://cmake.org/files/v3.15/cmake-3.18.2.tar.gz && \
    tar -xvzf cmake-3.18.2.tar.gz && \
    cd cmake-3.18.2 && \
    export CC=clang-9; export CXX=clang++-9 && \
    ./bootstrap && \
    make -j$(nproc) && \
    make install

###### build E3
RUN git clone https://github.com/momalab/e3.git && \
    cd e3/src && \
    make -j$(nproc)

###### build target libraries
WORKDIR /e3/3p
RUN find *.sh -type f -exec sed -i 's/make/make -j$(nproc)/' {} \; && \
    ./fhew_unx.sh && \
    ./heli_unx.sh && \
    ./mpir_unx.sh && \
    ./seal_unx.sh && \
    ./tfhe_unx.sh

CMD ["/bin/bash"]
