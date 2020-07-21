FROM ubuntu:20.04

# install dependencies
RUN apt-get update
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get -y install opam cmake flex bison
RUN apt-get -y install build-essential git

###### install the AWS CLI required for result upload to S3
RUN apt-get -y install awscli

# required for Docker, see https://github.com/ocaml/opam/issues/3498
RUN opam init -y --disable-sandboxing
RUN opam install -y ocamlfind ocamlgraph batteries ocamlbuild

# clone and build Lobster
WORKDIR /root
RUN git clone https://github.com/ropas/PLDI2020_242_artifact_publication.git Lobster
WORKDIR /root/Lobster
RUN git checkout 8edefad221fc9f31612a3377cf222f724de3953a
RUN eval $(opam env); ./build.sh

CMD ["/bin/bash"]
