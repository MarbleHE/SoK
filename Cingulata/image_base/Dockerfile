#######################################
# Cingulata with in-house B/FV SHE implementation
#
# docker build -t cingulata:bfv .
# docker run -it --rm cingulata:bfv
#######################################

FROM ubuntu:18.04

# install dependencies
RUN ln -snf /usr/share/zoneinfo/$(curl https://ipapi.co/timezone) /etc/localtime
RUN apt-get update -qq \
 && apt-get install --no-install-recommends -y \
    ca-certificates \
    cmake \
    curl \
    g++ \
    git \
    libboost-graph-dev \
    libboost-program-options-dev \
    libflint-dev \
    libpugixml-dev \
    make \
    python3 \
    python3-networkx \
    python3-numpy \
    tzdata \
    xxd \ 
    yad \
    rename \
    xmlstarlet

###### install the AWS CLI required for result upload to S3
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get -y install awscli

# clone and compile TFHE library
RUN git clone https://github.com/tfhe/tfhe /tfhe \
 && cd tfhe \
 && make

# clone the Cingulata repository at a fixed revision
WORKDIR /
RUN git clone --recurse-submodules -j8 https://github.com/CEA-LIST/Cingulata.git cingu
WORKDIR /cingu
RUN git checkout eb028240000ee4d4cbc7b1a1aacfaf62ca1f6e5f

# compile cingulata BFV
RUN mkdir -p /cingu/build_bfv \
 && cd /cingu/build_bfv \
 && cmake -DUSE_BFV=ON -j $(nproc) .. \
 && make

# compile cingulata TFHE
RUN mkdir -p /cingu/build_tfhe \
 && cd /cingu/build_tfhe \
 && cmake -DTFHE_PATH=/tfhe -j $(nproc) .. \
 && make

CMD ["/bin/bash"]
