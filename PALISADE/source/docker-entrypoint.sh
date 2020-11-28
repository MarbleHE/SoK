#!/bin/bash

# Run example program: image_processing
cd /root/eval \
    && mkdir build \
    && cd build \
    && cmake .. \
    && make \
    && ./microbenchmark

