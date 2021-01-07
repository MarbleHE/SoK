FROM marblehe/base_cingulata

# copy eval program into container
COPY source/ /cingu/eval

# copy TFHE examples into Cingulata tree and rebuild
RUN cp -r  /cingu/eval/cardio-cingulata-tfhe /cingu/tests/tfhe/cardio \
 && cp -r  /cingu/eval/chi-squared-cingulata-tfhe /cingu/tests/tfhe/chi-squared \
 && cd /cingu/build_tfhe \
 && cmake -DTFHE_PATH=/tfhe -j $(nproc) .. \
 && make

# make entrypoint script executable
WORKDIR /cingu/eval
RUN chmod +x docker-entrypoint.sh

# execute the benchmark and upload benchmark results to S3
ENTRYPOINT ["/cingu/eval/docker-entrypoint.sh"]
