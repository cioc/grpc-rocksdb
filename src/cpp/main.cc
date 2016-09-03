#include "keyvalueimpl.h"
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <iostream>
#include <stdio.h>
#include <string>

using grpc::Server;
using grpc::ServerBuilder;

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cout << "Incorrect arguments supplied: grpc-rocksdb <addr> <storagePath>";
        return 1;
    }
    std::string server_addr(argv[1]);
    
    KeyValueImpl service(argv[2]);

    ServerBuilder builder;
    builder.AddListeningPort(server_addr, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_addr << std::endl;
    server->Wait();   

    return 0;
}
