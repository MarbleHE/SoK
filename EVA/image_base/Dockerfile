FROM ubuntu:20.04 as base_eva

ENV ROOT_HOME /

###### install all required packages
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update; \ 
    apt-get -y install git wget awscli build-essential clang libmsgsl-dev zlib1g-dev libssl-dev libboost-all-dev libprotobuf-dev protobuf-compiler python3-pip; \
    update-alternatives --install /usr/bin/cc cc /usr/bin/clang 100; \
    update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++ 100

 
###### build cmake from source (to get a new enough version for SEAL)
RUN wget https://cmake.org/files/v3.19/cmake-3.19.1.tar.gz && \
    tar -xvzf cmake-3.19.1.tar.gz && \
    cd cmake-3.19.1 && \
    export CC=clang; export CXX=clang++ && \
    ./bootstrap && \
    make -j$(nproc) && \
    make install

###### build and install SEAL
WORKDIR $ROOT_HOME
RUN git clone --branch 3.6.0 https://github.com/microsoft/SEAL.git && \
    mkdir -p SEAL/build; \
    cd SEAL/build; \
    export CC=clang; export CXX=clang++; \
    cmake -DCMAKE_BUILD_TYPE=Release -DSEAL_THROW_ON_TRANSPARENT_CIPHERTEXT=OFF -DUSE_GALOIS=ON .. && \
    make -j$(nproc) && \
    make install

###### build and install EVA
WORKDIR $ROOT_HOME
RUN git clone https://github.com/microsoft/EVA.git && \
    cd EVA; \
    git submodule update --init; \
    cmake . && \
    make -j$(nproc) && \
    python3 -m pip install -e python/ && \
    python3 -m pip install -r examples/requirements.txt

CMD ["/bin/bash"]
