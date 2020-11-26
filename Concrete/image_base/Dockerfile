FROM ubuntu:20.04 as base_concrete

ENV ROOT_HOME /

###### install all required packages and Rust
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update; \ 
    apt-get -y install git wget awscli build-essential cmake make libfftw3-dev libssl-dev cargo pkg-config; \
    curl --tlsv1.2 -sSf https://sh.rustup.rs | sh

# no other steps required as the Rust package for concrete will automatically be downloaded
# by Rust's package manager cargo

CMD ["/bin/bash"]
