#include "keyvalue.grpc.pb.h"
#include "keyvalueimpl.h"
#include "rocksdb/db.h"
#include "rocksdb/utilities/transaction.h"
#include "rocksdb/utilities/transaction_db.h"
#include <algorithm>
#include <assert.h>
#include <dirent.h>
#include <grpc++/security/server_credentials.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc/grpc.h>
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unordered_map>

using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;
using keyvalue::KeyValue;
using rocksdb::TransactionDB;

const std::string KeyValueImpl::NO_ERROR = "";
const std::string KeyValueImpl::TABLE_ALREADY_EXISTS = "Table already exists";
const std::string KeyValueImpl::TABLE_DOES_NOT_EXIST = "Table does not exist";
const std::string KeyValueImpl::DISK_ERROR = "Disk error";
const std::string KeyValueImpl::INTERNAL_ERROR = "Internal error";
const std::string KeyValueImpl::KEY_NOT_FOUND = "Key not found";
const std::string KeyValueImpl::CONDITION_NOT_MET = "Condition not met";

grpc::StatusCode KeyValueImpl::ToStatusCode(keyvalue::ErrorCode err) {
    switch (err) {
        case keyvalue::ErrorCode::TABLE_DOES_NOT_EXIST:
            return grpc::StatusCode::NOT_FOUND;
        case keyvalue::ErrorCode::KEY_NOT_FOUND:
            return grpc::StatusCode::NOT_FOUND;
        case keyvalue::ErrorCode::INTERNAL_ERROR:
            return grpc::StatusCode::INTERNAL;
        default:
            return grpc::StatusCode::INTERNAL;
    }
}

std::string KeyValueImpl::ToErrorMsg(keyvalue::ErrorCode err) {
    switch (err) {
        case keyvalue::ErrorCode::TABLE_DOES_NOT_EXIST:
            return TABLE_DOES_NOT_EXIST;
        case keyvalue::ErrorCode::KEY_NOT_FOUND:
            return KEY_NOT_FOUND;
        case keyvalue::ErrorCode::INTERNAL_ERROR:
            return NO_ERROR;
        default:
            return NO_ERROR; 
    }
}

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
    std::string topLevelName = dbDir + "/" + req->tablename();
    std::string tableName = topLevelName + "/" + tablePrivateName;
    
    //Check if top level exists, create if necessary
    DIR* dir = opendir(topLevelName.c_str());
    if (dir) {
        closedir(dir);
    } else if (ENOENT == errno){
        const int dir_err = mkdir(topLevelName.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if (dir_err == -1) {
            mtx.unlock();
            res->set_errorcode(keyvalue::ErrorCode::DISK_ERROR); 
            return Status(grpc::StatusCode::RESOURCE_EXHAUSTED, DISK_ERROR + " failed to create top level directory"); 
        }
    } else {
        mtx.unlock();
        res->set_errorcode(keyvalue::ErrorCode::DISK_ERROR); 
        return Status(grpc::StatusCode::RESOURCE_EXHAUSTED, DISK_ERROR + " failed to check for top level directory"); 
    }

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
        return Status(grpc::StatusCode::RESOURCE_EXHAUSTED, DISK_ERROR + " " + status.ToString()); 
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
    
    rocksdb::Status s = rocksdb::Status::OK();
    keyvalue::ErrorCode err = keyvalue::ErrorCode::NONE;
    rocksdb::TransactionDB* txnDb;
    keyvalue::Item item;
    std::string value; 

    mtx.lock_shared();
    
    auto lookup = tableLookup.find(req->tablename());
    
    if (lookup == tableLookup.end()) {
        err = keyvalue::ErrorCode::TABLE_DOES_NOT_EXIST;
        goto HANDLE_ERROR;
    }
    
    txnDb = lookup->second;
    
    s = txnDb->Get(readOptions, req->key(), &value);
    if (!s.ok()) { goto HANDLE_ERROR; }
    
    item.set_key(req->key());
    item.set_value(value);
        
    res->mutable_item()->CopyFrom(item);
HANDLE_ERROR:
    mtx.unlock_shared();
    
    if (err == keyvalue::ErrorCode::NONE && s.ok()) {
        res->set_errorcode(keyvalue::ErrorCode::NONE);
        return Status::OK;
    }

    if (err != keyvalue::ErrorCode::NONE) {
        res->set_errorcode(err);
        return Status(ToStatusCode(err), ToErrorMsg(err));
    }

    if (s.IsNotFound()) {
        err = keyvalue::ErrorCode::KEY_NOT_FOUND;
    } else {
        err = keyvalue::ErrorCode::INTERNAL_ERROR;   
    }
    
    return Status(ToStatusCode(err), ToErrorMsg(err) + " " + s.ToString());
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
