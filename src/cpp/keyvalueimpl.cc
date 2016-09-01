#include "keyvalue.grpc.pb.h"
#include "keyvalueimpl.h"
#include "rocksdb/db.h"
#include "rocksdb/utilities/transaction.h"
#include "rocksdb/utilities/transaction_db.h"
#include <algorithm>
#include <assert.h>
#include <grpc++/security/server_credentials.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc/grpc.h>
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;
using keyvalue::KeyValue;
using rocksdb::TransactionDB;

const std::string KeyValueImpl::TABLE_ALREADY_EXISTS = "Table already exists";
const std::string KeyValueImpl::TABLE_DOES_NOT_EXIST = "Table does not exist";
const std::string KeyValueImpl::DISK_ERROR = "Disk error";
const std::string KeyValueImpl::INTERNAL_ERROR = "Internal error";
const std::string KeyValueImpl::KEY_NOT_FOUND = "Key not found";
const std::string KeyValueImpl::CONDITION_NOT_MET = "Condition not met";

KeyValueImpl::KeyValueImpl() {
    dbDir = "/tmp";         
    shuffleSource = "0123456789abcdefghijklmnopqrstuvwxyz";
    txnOptions.set_snapshot = true;
     
    // If the tables table doesn't exist, create it.
    rocksdb::Options options;
    options.create_if_missing = true;
    rocksdb::TransactionDBOptions txnDbOptions;
    rocksdb::Status status = TransactionDB::Open(options, txnDbOptions, dbDir + "/tables", &tablesTable);
    assert(status.ok());

}

Status KeyValueImpl::CreateTable(ServerContext* context,
                                 const keyvalue::CreateTableReq* req,
                                 keyvalue::CreateTableRes* res) {
    mtx.lock();
    
    auto lookup = tableLookup.find(req->tablename());
    
    if (lookup != tableLookup.end()) {
        mtx.unlock();
        res->set_errorcode(keyvalue::ErrorCode::TABLE_ALREADY_EXISTS);
        return Status(grpc::StatusCode::ALREADY_EXISTS, TABLE_ALREADY_EXISTS);
    }
    
    //Table doesn't exist

    //Generate the private name of the table
    std::string tablePrivateName = shuffleSource;
    std::random_shuffle(tablePrivateName.begin(), tablePrivateName.end()); 
    std::string tableName = dbDir + "/" + req->tablename() + "/" + tablePrivateName;

    //Create the table on disk
    rocksdb::Options options;
    options.create_if_missing = true;
    rocksdb::TransactionDBOptions txnOptions;
    TransactionDB* newTable;
    rocksdb::Status status = TransactionDB::Open(options, txnOptions, tableName, &newTable);
    
    if (!status.ok()) {
        //Failed to create table on disk
        mtx.unlock();
        res->set_errorcode(keyvalue::ErrorCode::DISK_ERROR); 
        return Status(grpc::StatusCode::RESOURCE_EXHAUSTED, DISK_ERROR); 
    }
    
    //Insert the table into the tables table
    rocksdb::DB* db = tablesTable->GetBaseDB();
    db->Put(writeOptions, req->tablename(), tableName);   
    
    // Install into the tableLookup
    tableLookup[req->tablename()] = newTable;
             
    mtx.unlock();
    res->set_errorcode(keyvalue::ErrorCode::NONE);
    return Status::OK;
}

Status KeyValueImpl::DeleteTable(ServerContext* context,
                                 const keyvalue::DeleteTableReq* req,
                                 keyvalue::DeleteTableRes* res) {
    mtx.lock();

    auto lookup = tableLookup.find(req->tablename());
    
    if (lookup == tableLookup.end()) {
        //Table doesn't exist
        mtx.unlock();
        res->set_errorcode(keyvalue::ErrorCode::TABLE_DOES_NOT_EXIST);
        return Status(grpc::StatusCode::NOT_FOUND, TABLE_DOES_NOT_EXIST);
    }
    
    //Table exists
    rocksdb::TransactionDB* txnDb = lookup->second;
    
    tableLookup.erase(req->tablename()); 
    
    delete txnDb;
    
    //TODO - clean up the on disk files
     
    mtx.unlock(); 
    res->set_errorcode(keyvalue::ErrorCode::NONE);
    return Status::OK; 
}

Status KeyValueImpl::Get(ServerContext* context,
                         const keyvalue::GetReq* req,
                         keyvalue::GetRes* res) {
    mtx.lock_shared();
    
    auto lookup = tableLookup.find(req->tablename());
    
    if (lookup == tableLookup.end()) {
        //Table doesn't exist
        mtx.unlock_shared();
        res->set_errorcode(keyvalue::ErrorCode::TABLE_DOES_NOT_EXIST);
        return Status(grpc::StatusCode::NOT_FOUND, TABLE_DOES_NOT_EXIST);
    }
    
    //Table exists
    rocksdb::TransactionDB* txnDb = lookup->second;
    
    std::string value; 
    rocksdb::Status readStatus = txnDb->Get(readOptions, req->key(), &value);

    if (!readStatus.ok()) {
        mtx.unlock_shared();

        if (readStatus.IsNotFound()) {
            res->set_errorcode(keyvalue::ErrorCode::KEY_NOT_FOUND);
            return Status(grpc::StatusCode::NOT_FOUND, KEY_NOT_FOUND);        
        } 

        res->set_errorcode(keyvalue::ErrorCode::INTERNAL_ERROR);
        return Status(grpc::StatusCode::INTERNAL, INTERNAL_ERROR);
    }
    
    keyvalue::Item item;
    item.set_key(req->key());
    item.set_value(value);
        
    res->mutable_item()->CopyFrom(item);
      
    mtx.unlock_shared();
    res->set_errorcode(keyvalue::ErrorCode::NONE);
    return Status::OK;
}

Status KeyValueImpl::Put(ServerContext* context,
                         const keyvalue::PutReq* req,
                         keyvalue::PutRes* res) {
    mtx.lock_shared();
    
    auto lookup = tableLookup.find(req->tablename());
    
    if (lookup == tableLookup.end()) {
        //Table doesn't exist
        mtx.unlock_shared();
        res->set_errorcode(keyvalue::ErrorCode::TABLE_DOES_NOT_EXIST);
        return Status(grpc::StatusCode::NOT_FOUND, TABLE_DOES_NOT_EXIST);
    }
    
    //Table exists
    rocksdb::TransactionDB* txnDb = lookup->second;
    
    if (req->condition().size() > 0) {
        auto txn = txnDb->BeginTransaction(writeOptions);
        rocksdb::ReadOptions myReadOptions;
        myReadOptions.snapshot = txn->GetSnapshot();
        std::string currentValue; 
        rocksdb::Status readStatus = txn->GetForUpdate(myReadOptions, req->item().key(), &currentValue);
        
        if (!readStatus.ok()) {
            mtx.unlock_shared();
            delete txn;

            if (readStatus.IsNotFound()) {
                res->set_errorcode(keyvalue::ErrorCode::KEY_NOT_FOUND);
                return Status(grpc::StatusCode::NOT_FOUND, KEY_NOT_FOUND);        
            } 
            
            res->set_errorcode(keyvalue::ErrorCode::INTERNAL_ERROR);
            return Status(grpc::StatusCode::INTERNAL, INTERNAL_ERROR);
        }

        if (currentValue.compare(req->condition()) != 0) {
            mtx.unlock_shared();
            delete txn;
            res->set_errorcode(keyvalue::ErrorCode::CONDITION_NOT_MET);
            return Status(grpc::StatusCode::ABORTED, CONDITION_NOT_MET);
        }
        
        txn->Put(req->item().key(), req->item().value());  
        rocksdb::Status txnStatus = txn->Commit(); 
        delete txn;
        
        if (!txnStatus.ok()) {
            mtx.unlock_shared();
            res->set_errorcode(keyvalue::ErrorCode::CONDITION_NOT_MET);
            return Status(grpc::StatusCode::ABORTED, CONDITION_NOT_MET);
        }
    } else {
        rocksdb::Status putStatus = txnDb->Put(writeOptions, req->item().key(), req->item().value());
        
        if (!putStatus.ok()) {
            mtx.unlock_shared();

            res->set_errorcode(keyvalue::ErrorCode::INTERNAL_ERROR);
            return Status(grpc::StatusCode::INTERNAL, INTERNAL_ERROR);
        }
    }
    
    mtx.unlock_shared();
    res->set_errorcode(keyvalue::ErrorCode::NONE);
    return Status::OK;
}

Status KeyValueImpl::Delete(ServerContext* context,
                            const keyvalue::DeleteReq* req,
                            keyvalue::DeleteRes* res) {
    return Status::OK;
}

Status KeyValueImpl::Range(ServerContext* context,
                           const keyvalue::RangeReq* req,
                           ServerWriter<keyvalue::RangeRes>* writer) {
    return Status::OK;
}

void RunServer() {
    std::string server_addr("0.0.0.0:8992");
    
    KeyValueImpl service;

    ServerBuilder builder;
    builder.AddListeningPort(server_addr, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_addr << std::endl;
    server->Wait();   
}
