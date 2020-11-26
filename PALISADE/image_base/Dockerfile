FROM ubuntu:20.04 as base_palisade

ENV ROOT_HOME /

###### install all required packages
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update; \ 
    apt-get -y install git wget awscli build-essential libomp-dev cmake make autoconf

# ###### build PALISADE
WORKDIR $ROOT_HOME
RUN git clone --branch v1.10.5 https://gitlab.com/palisade/palisade-release.git && \
    cd palisade-release; \
    git submodule sync --recursive && \
    git submodule update --init  --recursive && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make -j$(nproc) && \
    make install

CMD ["/bin/bash"]
