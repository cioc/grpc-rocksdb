#include <iostream>
#include <string>

#include <grpc/grpc.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc++/security/server_credentials.h>
#include "keyvalue.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;

using keyvalue::KeyValue;

class KeyValueImpl final : public KeyValue::Service {
//    public:
//        explicit KeyValueImpl() {
//             
//        } 
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
