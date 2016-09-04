#!/usr/bin/python

import grpc
import keyvalue_pb2 
import os
import sys

if __name__ == '__main__':
    conn_str = ""
    if len(sys.argv) == 2:
        conn_str = sys.argv[1] 
    else:
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
    try:
        del_res = stub.Delete(keyvalue_pb2.DeleteReq(tablename='test-table-1',key='myKey',condition="88"))
        print "Condition should not be met!"
        sys.exit(1)
    except Exception:
        pass
    del_res = stub.Delete(keyvalue_pb2.DeleteReq(tablename='test-table-1',key='myKey',condition="99999"))
    try:
        get_res = stub.Get(keyvalue_pb2.GetReq(tablename='test-table-1',key='myKey'))
        print "Key should not be found"
        sys.exit(1)
    except Exception:
        pass
    put_res = stub.Put(keyvalue_pb2.PutReq(tablename='test-table-1',item=keyvalue_pb2.Item(key='myKey', value='12345')))
    get_res = stub.Get(keyvalue_pb2.GetReq(tablename='test-table-1',key='myKey'))
    assert get_res.item.value == "12345"
    del_res = stub.Delete(keyvalue_pb2.DeleteReq(tablename='test-table-1',key='myKey'))
    try:
        get_res = stub.Get(keyvalue_pb2.GetReq(tablename='test-table-1',key='myKey'))
        print "Key should not be found"
        sys.exit(1)
    except Exception:
        pass
    put_res = stub.Put(keyvalue_pb2.PutReq(tablename='test-table-1',item=keyvalue_pb2.Item(key='key1', value='1')))
    put_res = stub.Put(keyvalue_pb2.PutReq(tablename='test-table-1',item=keyvalue_pb2.Item(key='key2', value='2')))
    put_res = stub.Put(keyvalue_pb2.PutReq(tablename='test-table-1',item=keyvalue_pb2.Item(key='key3', value='3')))
    put_res = stub.Put(keyvalue_pb2.PutReq(tablename='test-table-1',item=keyvalue_pb2.Item(key='key4', value='4')))
    put_res = stub.Put(keyvalue_pb2.PutReq(tablename='test-table-1',item=keyvalue_pb2.Item(key='key5', value='5')))
    items = stub.Range(keyvalue_pb2.RangeReq(tablename='test-table-1'))
    counter = 1
    for i in items:
        assert i.key == ('key' + str(counter))
        assert i.value == str(counter)
        counter += 1
    assert counter == 6
    items = stub.Range(keyvalue_pb2.RangeReq(tablename='test-table-1',start='key2'))
    counter = 2
    for i in items:
        assert i.key == ('key' + str(counter))
        assert i.value == str(counter)
        counter += 1
    assert counter == 6
    items = stub.Range(keyvalue_pb2.RangeReq(tablename='test-table-1',end='key3'))
    counter = 1
    for i in items:
        assert i.key == ('key' + str(counter))
        assert i.value == str(counter)
        counter += 1
    assert counter == 3
    items = stub.Range(keyvalue_pb2.RangeReq(tablename='test-table-1',start='key2',end='key5'))
    counter = 2
    for i in items:
        assert i.key == ('key' + str(counter))
        assert i.value == str(counter)
        counter += 1
    assert counter == 5
    delete_table_res = stub.DeleteTable(keyvalue_pb2.DeleteTableReq(tablename='test-table-1'))
