FROM marblehe/base_cingulata

# copy eval program into container
COPY source/eval /cingu/eval
COPY source/CMakeLists.txt /cingu/CMakeLists.txt

# build the benchmark
WORKDIR /cingu
RUN mkdir build; cd build; cmake ..; make

# execute the benchmark and upload benchmark results to S3
CMD ["sh", "-c", "cd /cingu/build/eval/cardio; ./run.sh; aws s3 cp cingulata_cardio.csv ${S3_URL}/${S3_FOLDER}/Cingulata/"]