#!/bin/bash

# Run microbenchmark
cd /root/eval \
    && mkdir build \
    && cd build \
    && cmake .. \
    && make \
    && ./microbenchmark

