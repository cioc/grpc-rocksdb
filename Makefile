CC = g++
CFLAGS = -Wall -std=c++11 -c 
LDFLAGS = -L. -lgrpc++_unsecure -lgrpc -lgpr -lrocksdb -lstdc++ -lbz2 -lz -lsnappy -lgrpc++_reflection -lprotobuf -lpthread -ldl

all: grpc-rocksdb 

grpc-rocksdb: main.o keyvalueimpl.o keyvalue.grpc.pb.o keyvalue.pb.o
		$(CC) -o grpc-rocksdb main.o keyvalueimpl.o keyvalue.grpc.pb.o keyvalue.pb.o $(LDFLAGS)

keyvalue.grpc.pb.o: protos
		$(CC) $(CFLAGS) src/cpp/keyvalue.grpc.pb.cc
				
keyvalue.pb.o: protos
		$(CC) $(CFLAGS) src/cpp/keyvalue.pb.cc

protos:
		protoc -I src/protos --grpc_out=./src/cpp --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` src/protos/keyvalue.proto
		protoc -I src/protos --cpp_out=./src/cpp src/protos/keyvalue.proto

main.o:
		$(CC) $(CFLAGS) src/cpp/main.cc

keyvalueimpl.o: protos
		$(CC) $(CFLAGS) src/cpp/keyvalueimpl.cc
