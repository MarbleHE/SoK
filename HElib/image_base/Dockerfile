# This Dockerfile is based on HElib's official Dockerimage
# See https://github.com/homenc/HElib/blob/master/Dockerfile @ b3bf83fa23df97590de72eb735b9a74aa9ff6c3d
FROM ubuntu:20.04 as base_helib

# Install required Ubuntu packages
ENV DEBIAN_FRONTEND=noninteractive
RUN apt update && \
    apt install -y build-essential wget git cmake m4 libgmp-dev file

# Install Shoup's NTL library
RUN cd ~ && \
    wget https://www.shoup.net/ntl/ntl-11.4.1.tar.gz && \
    tar --no-same-owner -xf ntl-11.4.1.tar.gz && \
    cd ntl-11.4.1/src && \
    ./configure SHARED=on NTL_GMP_LIP=on NTL_THREADS=on NTL_THREAD_BOOST=on && \
    make -j$(nproc) && \
    make install

# Install HElib
RUN cd ~ && \
    git clone https://github.com/homenc/HElib.git && \
    mkdir HElib/build && \
    cd HElib/build && \
    cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED=ON -DENABLE_THREADS=ON .. && \
    make -j$(nproc) && \
    make install && \
    ldconfig

