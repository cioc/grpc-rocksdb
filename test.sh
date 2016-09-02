#!/bin/bash

#Build the server container
docker build -t grpc-rocksdb/latest .

#Build the test container
mkdir -p ./tst/protos
cp src/protos/* ./tst/protos
cd ./tst
docker build -t grpc-rocksdb/test .

#Cleanup anything that may have been left running
docker kill grpcrocksdb
docker kill grpcrocksdb-test
docker rm grpcrocksdb
docker rm grpcrocksdb-test

#Run the tests
docker run -d --expose=8992 -p 8992:8992 -v /tmp --name=grpcrocksdb grpc-rocksdb/latest ./build/grpc-rocksdb
docker run -a stdin -a stdout -a stderr -i -t --sig-proxy=true --link grpcrocksdb --name=grpcrocksdb-test grpc-rocksdb/test ./build/test.py

#Cleanup
docker kill grpcrocksdb
docker kill grpcrocksdb-test
