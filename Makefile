CC = g++
CFLAGS = -Wall -std=c++11 -c 
LFLAGS = -L. -lrocksdb -lstdc++ -lbz2 -lz -lsnappy

all: grpc-rocksdb 

grpc-rocksdb: main.o protos
		$(CC) -o grpc-rocksdb main.o $(LFLAGS)

protos:
		protoc -I src/protos --grpc_out=./src/cpp --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` src/protos/helloworld.proto
		protoc -I src/protos --cpp_out=./src/cpp src/protos/helloworld.proto

main.o:
		$(CC) $(CFLAGS) src/cpp/main.cc
