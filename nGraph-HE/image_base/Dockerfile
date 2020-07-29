# ******************************************************************************
# Copyright 2018-2020 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ******************************************************************************

# Source: https://github.com/IntelAI/he-transformer/blob/master/contrib/docker/Dockerfile.he_transformer.ubuntu1804

# Environment to build and unit-test he-transformer
# with g++ 7.4.0
# with clang++ 9.0.1
# with python 3.6.8
# with cmake 3.14.4

FROM ubuntu:18.04

RUN apt-get update && apt-get install -y \
    python3-pip virtualenv python3-dev \
    git \
    unzip wget \
    sudo \
    bash-completion \
    build-essential cmake \
    software-properties-common \
    git \
    wget patch diffutils libtinfo-dev \
    autoconf libtool \
    doxygen graphviz \
    yapf3 python3-yapf \
    python python-dev python3 python3-dev \
    libomp-dev

RUN python3.6 -m pip install pip --upgrade && \ 
    pip3 install -U --user pip six 'numpy<1.19.0' wheel setuptools mock 'future>=0.17.1' && \ 
    pip3 install -U --user keras_applications --no-deps && \ 
    pip3 install -U --user keras_preprocessing --no-deps && \ 
    rm -rf /usr/bin/python && \
    ln -s /usr/bin/python3.6 /usr/bin/python

# Install clang-9
RUN wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -
RUN apt-add-repository "deb http://apt.llvm.org/bionic/ llvm-toolchain-bionic-9 main"
RUN apt-get update && apt install -y clang-9 clang-tidy-9 clang-format-9

RUN apt-get update && apt-get install -y gcc-4.8 g++-4.8
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.8 60 --slave /usr/bin/g++ g++ /usr/bin/g++-4.8 && \
  update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-7 60 --slave /usr/bin/g++ g++ /usr/bin/g++-7

RUN apt-get clean autoclean && apt-get autoremove -y

# For ngraph-tf integration testing
RUN pip3 install --upgrade pip setuptools virtualenv==16.1.0

# SEAL requires newer version of CMake
RUN pip3 install cmake --upgrade

# Get bazel for ng-tf
RUN wget https://github.com/bazelbuild/bazel/releases/download/0.25.2/bazel-0.25.2-installer-linux-x86_64.sh
RUN chmod +x ./bazel-0.25.2-installer-linux-x86_64.sh
RUN bash ./bazel-0.25.2-installer-linux-x86_64.sh
WORKDIR /home

# *** end of Dockerfile from IntelAI/he-transformer repository ****************

ENV HE_TRANSFORMER /home/he-transformer

# Build HE-Transformer
# https://github.com/IntelAI/he-transformer#1-build-he-transformer
WORKDIR /home
RUN git clone https://github.com/IntelAI/he-transformer.git he-transformer

WORKDIR $HE_TRANSFORMER
COPY ngraph-tf.cmake /cmake/

RUN mkdir build && \
    cd build && \ 
    cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++-9 -DCMAKE_C_COMPILER=clang-9 -Werror
WORKDIR $HE_TRANSFORMER/build
RUN env VERBOSE=1 make -j 144 install

# Build the Python bindings for client
# https://github.com/IntelAI/he-transformer#1c-python-bindings-for-client
RUN cd $HE_TRANSFORMER/build && \
    source external/venv-tf-py3/bin/activate && \
    make install python_client && \
    pip install python/dist/pyhe_client-*.whl && \
    python3 -c "import pyhe_client"

CMD ["/bin/bash"]


