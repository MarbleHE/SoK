#!/bin/bash

# Run example program: image_processing
cd /root/eval/image_processing \
    && python3 -m pip install -r requirements.txt \
    && python3 image_processing.py 

# Run benchmark program: chi_squared
cd /root/eval/chi_squared \
    && python3 -m pip install -r requirements.txt \
    && python3 chi_squared.py 
