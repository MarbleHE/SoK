FROM ubuntu:20.04 as base_seal

ENV ROOT_HOME /

###### install all required packages
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update; \ 
    apt-get -y install git wget awscli build-essential clang-tools-9 libmsgsl-dev zlib1g-dev
 
###### build cmake from source (to get a new enough version for SEAL)
RUN wget https://cmake.org/files/v3.15/cmake-3.15.0.tar.gz && \
    tar -xvzf cmake-3.15.0.tar.gz && \
    cd cmake-3.15.0 && \
    export CC=clang-9; export CXX=clang++-9 && \
    ./bootstrap && \
    make -j$(nproc) && \
    make install

###### build SEAL
WORKDIR $ROOT_HOME
RUN git clone --branch 3.5.6 https://github.com/microsoft/SEAL.git && \
    mkdir -p SEAL/build; cd SEAL/build; export CC=clang-9; export CXX=clang++-9; \
    cmake -DSEAL_BUILD_SEAL_C=ON -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release .. && \
    make -j$(nproc) && \
    make install

CMD ["/bin/bash"]
