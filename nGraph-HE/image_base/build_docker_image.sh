#!/bin/bash

# This script can be used to manually build the docker image on the EC2 VM.

# check if the docker command was found (assumes that docker is installed)
if getent group docker | grep -q "\b$USER\b"; then
then
  echo "Run manually following commands prior executing this script to add the current user to the docker group:"
  echo "  sudo usermod -aG docker $USER"
  echo "  newgrp docker"
  exit 1
fi

# build image
TS=`date +%s`
DOCKER_BUILDKIT=1 docker build -t base_ngraph-he . 2>&1 | tee docker-build-${TS}.log
