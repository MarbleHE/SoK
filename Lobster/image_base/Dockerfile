FROM ubuntu:20.04

# install dependencies
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update; apt-get -y install opam cmake flex bison build-essential git awscli

# required for Docker, see https://github.com/ocaml/opam/issues/3498
RUN opam init -y --disable-sandboxing
RUN opam install -y ocamlfind ocamlgraph batteries ocamlbuild

# clone and build Lobster
WORKDIR /root
RUN git clone https://github.com/ropas/PLDI2020_242_artifact_publication.git Lobster \ 
    && cd Lobster \
    && git checkout 8edefad221fc9f31612a3377cf222f724de3953a \
    && eval $(opam env) \
    && ./build.sh

CMD ["/bin/bash"]
