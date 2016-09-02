FROM ubuntu:16.04

RUN apt-get update
RUN apt-get install -y build-essential autoconf libtool curl git

RUN git clone -b $(curl -L http://grpc.io/release) https://github.com/grpc/grpc \
    && cd grpc \
    && git submodule update --init \
    && make \
    && make install \
    && cd third_party/protobuf \
    && make \
    && make install

RUN apt-get install -y python-setuptools python-dev \
    && easy_install pip \
    && pip install --upgrade virtualenv

RUN pip install grpcio \
    && pip install grpcio-tools

RUN mkdir -p /build
COPY protos /build
COPY test.py /build
COPY build.sh /build

RUN cd /build \
    && ./build.sh 
