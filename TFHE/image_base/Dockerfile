FROM ubuntu:20.04

RUN ln -snf /usr/share/zoneinfo/$(curl https://ipapi.co/timezone) /etc/localtime
RUN apt-get update && \
    apt-get install -y build-essential clang libomp-dev cmake git fftw3 && \
    git clone --recursive https://github.com/tfhe/tfhe.git && \
    mkdir /tfhe/build

WORKDIR /tfhe/build

# FFTW3 is the fastest FFT implementation, see https://github.com/tfhe/tfhe#dependencies for details
# Note that there are no TFHE tests (existing tests are only for the FFT implementations Nayuki and Spqlios)
RUN cmake ../src -DENABLE_TESTS=off -DENABLE_NAYUKI_PORTABLE=off -DENABLE_NAYUKI_AVX=off -DENABLE_SPQLIOS_AVX=off -DENABLE_SPQLIOS_FMA=off -DCMAKE_BUILD_TYPE=optim && \
    make && \
    make install

CMD ["/bin/bash"]