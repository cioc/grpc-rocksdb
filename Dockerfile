FROM ubuntu:16.04

RUN apt-get update
RUN apt-get install -y build-essential autoconf libtool curl git
RUN git clone -b $(curl -L http://grpc.io/release) https://github.com/grpc/grpc \
    && cd grpc \
    && git submodule update --init \
    && make \
    && make install
