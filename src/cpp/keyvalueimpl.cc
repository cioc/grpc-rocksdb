#include "keyvalue.grpc.pb.h"
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
#include <map>
#include <mutex>
#include <shared_mutex>
#include <string>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;
using keyvalue::KeyValue;
using rocksdb::TransactionDB;

class KeyValueImpl final : public KeyValue::Service {
public:
    explicit KeyValueImpl() {
        dbDir = "/tmp";         
        shuffleSource = "0123456789abcdefghijklmnopqrstuvwxyz";
         
        // If the tables table doesn't exist, create it.
        rocksdb::Options options;
        options.create_if_missing = true;
        rocksdb::TransactionDBOptions txnOptions;
        rocksdb::Status status = TransactionDB::Open(options, txnOptions, dbDir + "/tables", &tablesTable);
        assert(status.ok());
    }

    Status CreateTable(ServerContext* context,
                       const keyvalue::CreateTableReq* req,
                       keyvalue::CreateTableRes* res) override {
        mtx.lock();
        
        auto lookup = tableLookup.find(req->name());
        
        if (lookup != tableLookup.end()) {
            //Table already exists
            mtx.unlock();
            return Status::OK;
        }
        
        //Table doesn't exist

        //Generate the private name of the table
        std::string tablePrivateName = shuffleSource;
        std::random_shuffle(tablePrivateName.begin(), tablePrivateName.end()); 
        std::string tableName = dbDir + "/" + req->name() + "/" + tablePrivateName;

        //Create the table on disk
        rocksdb::Options options;
        options.create_if_missing = true;
        rocksdb::TransactionDBOptions txnOptions;
        TransactionDB* newTable;
        rocksdb::Status status = TransactionDB::Open(options, txnOptions, tableName, &newTable);
        assert(status.ok());
        
        //Insert the table into the tables table
        rocksdb::DB* db = tablesTable->GetBaseDB();
        db->Put(writeOptions, req->name(), tableName);   
        
        // Install into the tableLookup
        tableLookup[req->name()] = newTable;
                 
        mtx.unlock();
        return Status::OK;
    }

    Status DeleteTable(ServerContext* context,
                       const keyvalue::DeleteTableReq* req,
                       keyvalue::DeleteTableRes* res) override {
        return Status::OK; 
    }

    Status Get(ServerContext* context,
               const keyvalue::GetReq* req,
               keyvalue::GetRes* res) override {
        return Status::OK;
    }

    Status Put(ServerContext* context,
               const keyvalue::PutReq* req,
               keyvalue::PutRes* res) override {
        return Status::OK;
    }

    Status Delete(ServerContext* context,
                  const keyvalue::DeleteReq* req,
                  keyvalue::DeleteRes* res) override {
        return Status::OK;
    }
    
    Status Range(ServerContext* context,
                 const keyvalue::RangeReq* req,
                 ServerWriter<keyvalue::RangeRes>* writer) override {
        return Status::OK;
    }

private:
    std::shared_timed_mutex mtx;
    TransactionDB* tablesTable;
    std::map<std::string, TransactionDB*> tableLookup;
    std::string dbDir; 
    std::string shuffleSource;
    rocksdb::WriteOptions writeOptions;
};

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
