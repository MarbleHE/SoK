FROM haskell:8

###### install the AWS CLI required for result upload to S3
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update; apt-get -y install awscli

###### get the ALCHEMY code
WORKDIR /root
RUN git clone https://github.com/cpeikert/ALCHEMY.git
# using the latest commit it was not possible to execute the example due to an error regarding rescaleTree (added a few commits before)
RUN cd ALCHEMY && git checkout ad849d7ef6f5e7bd1467827e159e56d5120179c4

###### build ALCHEMY
WORKDIR /root/ALCHEMY
RUN stack setup
RUN stack build --no-terminal --install-ghc

CMD ["/bin/bash"]