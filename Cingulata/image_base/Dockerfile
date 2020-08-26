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
    rename

###### install the AWS CLI required for result upload to S3
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get -y install awscli

ENV build_dir=build_bfv

# clone the Cingulata repository at a fixed revision
WORKDIR /
RUN git clone --recurse-submodules -j8 https://github.com/CEA-LIST/Cingulata.git cingu
WORKDIR /cingu
RUN git checkout eb028240000ee4d4cbc7b1a1aacfaf62ca1f6e5f

# compile cingulata
RUN mkdir -p /cingu/$build_dir \
 && cd /cingu/$build_dir \
 && cmake -DUSE_BFV=ON .. \
 && make

CMD ["/bin/bash"]
