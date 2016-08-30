#include "keyvalue.grpc.pb.h"
#include <grpc++/security/server_credentials.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc/grpc.h>
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <string>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;
using keyvalue::KeyValue;

class KeyValueImpl final : public KeyValue::Service {
public:
    explicit KeyValueImpl() {
         
    }

    Status CreateTable(ServerContext* context,
                       const keyvalue::CreateTableReq* req,
                       keyvalue::CreateTableRes* res) override {
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
