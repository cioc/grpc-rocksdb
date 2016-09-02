#!/usr/bin/python

import grpc
import keyvalue_pb2 
import os

if __name__ == '__main__':
    conn_str = os.environ['GRPCROCKSDB_PORT'].split("/")[2]
    print "Connecting on: " + conn_str
    channel = grpc.insecure_channel(conn_str)
    print "1!"
    stub = keyvalue_pb2.KeyValueStub(channel)
    print "2!"
    create_table_res = stub.CreateTable(keyvalue_pb2.CreateTableReq(tablename='test-table-1'))
    print "3!"
    print create_table_res
