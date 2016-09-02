#!/usr/bin/python

import grpc
import keyvalue_pb2 
import os
import sys

if __name__ == '__main__':
    conn_str = os.environ['GRPCROCKSDB_PORT'].split("/")[2]
    print "Connecting on: " + conn_str
    channel = grpc.insecure_channel(conn_str)
    stub = keyvalue_pb2.KeyValueStub(channel)
    create_table_res = stub.CreateTable(keyvalue_pb2.CreateTableReq(tablename='test-table-1'))
    put_res = stub.Put(keyvalue_pb2.PutReq(tablename='test-table-1',item=keyvalue_pb2.Item(key='myKey', value='12345')))
    get_res = stub.Get(keyvalue_pb2.GetReq(tablename='test-table-1',key='myKey'))
    assert get_res.item.value == "12345"
    try:
        put_res = stub.Put(keyvalue_pb2.PutReq(tablename='test-table-1',item=keyvalue_pb2.Item(key='myKey', value='99999'),condition="hello"))
        print "Condition should not be met!"
        sys.exit(1)
    except Exception:
        pass
    get_res = stub.Get(keyvalue_pb2.GetReq(tablename='test-table-1',key='myKey'))
    assert get_res.item.value == "12345"
    put_res = stub.Put(keyvalue_pb2.PutReq(tablename='test-table-1',item=keyvalue_pb2.Item(key='myKey', value='99999'),condition="12345"))
    get_res = stub.Get(keyvalue_pb2.GetReq(tablename='test-table-1',key='myKey'))
    assert get_res.item.value == "99999"
