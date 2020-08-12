#!/bin/bash

docker build -t eval_seal . && \
docker run -it --rm -v $(pwd)/source:/root/eval:consistent eval_seal /bin/bash
