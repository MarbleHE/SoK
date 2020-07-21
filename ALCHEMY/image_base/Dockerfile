FROM haskell:8

###### install the AWS CLI required for result upload to S3
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update; apt-get -y install awscli

###### get the ALCHEMY code
WORKDIR /root
RUN git clone https://github.com/cpeikert/ALCHEMY.git
RUN cd ALCHEMY && git checkout adbef64576c6f6885600da66f59c5a4ad91810b7
COPY stack.yaml .

###### build ALCHEMY
WORKDIR /root/ALCHEMY
RUN stack build --no-terminal --install-ghc --fast

CMD ["/bin/bash"]