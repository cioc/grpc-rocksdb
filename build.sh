#!/bin/bash

set -x
set -e

function kill_container {
    docker kill $1
}

function delete_container {
    docker rm $1
}

function run_build {
    #Build the server container
    docker build -t grpc-rocksdb/latest .
}

function run_build_test {
    mkdir -p ./tst/protos
    cp src/protos/* ./tst/protos
    cd ./tst
    docker build -t grpc-rocksdb/test .
}

function run_server {
    docker run -d --expose=8992 -p 8992:8992 -v /tmp --name=grpcrocksdb grpc-rocksdb/latest ./build/grpc-rocksdb 0.0.0.0:8992 /tmp
}

function run_test {
    #Run the tests
    echo "TEST START"
    
    if [ -z "$1" ];
    then
        docker run -a stdin -a stdout -a stderr -i -t --sig-proxy=true --link grpcrocksdb --name=grpcrocksdb-test grpc-rocksdb/test ./build/test.py
    else
        docker run -a stdin -a stdout -a stderr -i -t --sig-proxy=true --name=grpcrocksdb-test grpc-rocksdb/test ./build/test.py $1
    fi 
    
    echo "TEST END"
}

function clean {
    #Cleanup anything that may have been left running
    kill_container grpcrocksdb || true
    kill_container grpcrocksdb-test || true
    docker rm grpcrocksdb || true
    docker rm grpcrocksdb-test || true
}

if [ $# -eq 0 ];
then 
    run_build
    run_build_test
    
    clean
    
    run_server
    run_test

    #Cleanup
    kill_container grpcrocksdb || true
    kill_container grpcrocksdb-test || true
    exit
fi

if [ $1 == "build" ];
then
    run_build
    exit
fi

if [ $1 == "test" ];
then
    run_test $2
    exit
fi

if [ $1 == "run" ];
then
    run_server
fi

if [ $1 == "build-test" ];
then
    run_build_test
fi

if [ $1 == "clean" ];
then
    clean
fi
