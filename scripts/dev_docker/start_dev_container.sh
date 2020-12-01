#!/bin/bash

# A script for mounting the "toolname/source/" directory to the container such that locally made changes are immediately
# visible in the Docker container.

# argument 1 ($1):
#	the name of the tool (e.g., SEAL) in lowercase (e.g., seal)
TOOLNAME=$(echo $1 | tr '[:upper:]' '[:lower:]')

# the location where your source files are located at
SRC_DIR=../e3/source

# the location where you want to map your files into (i.e., path in container)
DST_DIR=/eval

cd ../$1/image_base \
	&& docker run -it --rm -v ${SRC_DIR}:${DST_DIR}:consistent marblehe/base_${TOOLNAME} /bin/bash
