CC = g++
CFLAGS = -Wall

all:
		$(CC) $(CFLAGS) -o grpc-rocksdb main.cpp
