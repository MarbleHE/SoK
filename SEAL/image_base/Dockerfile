FROM ubuntu:20.04 as base_seal

ENV ROOT_HOME /

###### install wget git etc
RUN apt-get update; apt-get -y install git wget

###### install the AWS CLI required for result upload to S3
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get -y install awscli

###### install general build tools
RUN apt-get -y install build-essential

###### get clang++ as this is faster than gcc for SEAL
RUN apt-get -y install clang-tools-9

###### build cmake from source (to get a new enough version for SEAL)
RUN wget https://cmake.org/files/v3.15/cmake-3.15.0.tar.gz
RUN tar -xvzf cmake-3.15.0.tar.gz
WORKDIR cmake-3.15.0
RUN export CC=clang-9; export CXX=clang++-9; ./bootstrap; make -j8; make install

###### build optional SEAL dependencies (improve performance)
RUN apt-get -y install libmsgsl-dev
RUN apt-get -y install zlib1g-dev

###### build SEAL
WORKDIR $ROOT_HOME
RUN git clone --branch 3.5.6 https://github.com/microsoft/SEAL.git
WORKDIR SEAL
RUN export CC=clang-9; export CXX=clang++-9; cmake . -DSEAL_BUILD_SEAL_C=ON -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release; make -j64; make install

CMD ["/bin/bash"]