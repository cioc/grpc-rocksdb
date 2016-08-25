CC = g++
CFLAGS = -Wall -std=c++11 -c 
LFLAGS = -L. -lrocksdb -lstdc++ -lbz2 -lz -lsnappy

all: grpc-rocksdb 

grpc-rocksdb: main.o
		$(CC) -o grpc-rocksdb main.o $(LFLAGS)

main.o:
		$(CC) $(CFLAGS) src/cpp/main.cc
