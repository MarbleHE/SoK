#!/bin/bash

cd $1/image_base \
	&& docker run -it --rm -v $(pwd)/../source:/e3/eval:consistent marblehe/base_$(echo $1 | tr '[:upper:]' '[:lower:]') /bin/bash
