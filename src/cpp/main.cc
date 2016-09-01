#include "keyvalueimpl.h"
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <iostream>
#include <stdio.h>
#include <string>

using grpc::Server;
using grpc::ServerBuilder;

int main(int argc, char** argv) {
    std::string server_addr("0.0.0.0:8992");
    
    KeyValueImpl service;

    ServerBuilder builder;
    builder.AddListeningPort(server_addr, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_addr << std::endl;
    server->Wait();   

    return 0;
}
