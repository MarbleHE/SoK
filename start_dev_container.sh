#!/bin/bash

cd $1/image_base \
	&& docker run -it --rm -v $(pwd)/../source:/root/eval:consistent marblehe/base_$(echo $1 | tr '[:upper:]' '[:lower:]') /bin/bash
