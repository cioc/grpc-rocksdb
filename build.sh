#!/bin/bash

set -e

function kill_container {
  RUNNING_TEST=$(docker inspect -f {{.State.Running}} $1)
  if $RUNNING_TEST !== "false"
  then 
    docker kill $1
  fi
}

function delete_container {
  docker inspect -f {{.State.Running}} $1
  if [ $? -eq 0 ];
  then
    docker rm $1
  fi
}

function run_build {
    #Build the server container
    docker build -t grpc-rocksdb/latest .
}

function run_test {
    #Build the test container
    mkdir -p ./tst/protos
    cp src/protos/* ./tst/protos
    cd ./tst
    docker build -t grpc-rocksdb/test .

    #Cleanup anything that may have been left running
    kill_container grpcrocksdb
    kill_container grpcrocksdb-test
    docker rm grpcrocksdb
    docker rm grpcrocksdb-test

    #Run the tests
    echo "TEST START"
    echo "=========="
    echo ""
    docker run -d --expose=8992 -p 8992:8992 -v /tmp --name=grpcrocksdb grpc-rocksdb/latest ./build/grpc-rocksdb 0.0.0.0:8992 /tmp
    docker run -a stdin -a stdout -a stderr -i -t --sig-proxy=true --link grpcrocksdb --name=grpcrocksdb-test grpc-rocksdb/test ./build/test.py
    echo "TEST END"
    echo "========"
    echo ""

    #Cleanup
    kill_container grpcrocksdb
    kill_container grpcrocksdb-test
}

if [ $# -eq 0 ];
then 
    run_build
    run_test
    exit
fi

if [ $1 == "build" ];
then
    run_build
    exit
fi

if [ $1 == "test" ];
then
    run_test
    exit
fi
