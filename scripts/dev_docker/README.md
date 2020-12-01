# Docker Helper Scripts

This directory contains handy Docker scripts that we used during the development of this SoK:

`execute_benchmark_locally.sh` allows to run a benchmark locally before pushing it to AWS: It first configures the environment (e.g., sets the required environment variables), then builds the image, and finally executes it.

`start_dev_container.sh` facilitates the development of benchmark programs by mapping the source folder containing the benchmark programs into the Docker container.
