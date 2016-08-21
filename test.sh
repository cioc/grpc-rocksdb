#!/bin/bash
docker build -t grpc-rocksdb/latest .
docker run -i grpc-rocksdb/latest ./tmp/grpc-rocksdb
