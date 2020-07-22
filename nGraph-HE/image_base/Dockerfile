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
    python3-pip virtualenv \
    python3-numpy python3-dev python3-wheel \
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
    libomp-dev

# Install clang-9
RUN wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -
RUN apt-add-repository "deb http://apt.llvm.org/bionic/ llvm-toolchain-bionic-9 main"
RUN apt-get update && apt install -y clang-9 clang-tidy-9 clang-format-9

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

ENV HE_TRANSFORMER 

# Build HE-Transformer
# https://github.com/IntelAI/he-transformer#1-build-he-transformer
RUN git clone https://github.com/IntelAI/he-transformer.git
WORKDIR /home/he-transformer
RUN mkdir build && \
    cd build && \
    cmake .. -DCMAKE_CXX_COMPILER=clang++-9 && \
    make -j8 install

# Build the Python bindings for client
# https://github.com/IntelAI/he-transformer#1c-python-bindings-for-client
RUN cd /home/he-transformer/build && \
    source external/venv-tf-py3/bin/activate && \
    make install python_client && \
    pip install python/dist/pyhe_client-*.whl && \
    python3 -c "import pyhe_client"

CMD ["/bin/bash"]
