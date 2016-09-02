#!/bin/bash

python -m grpc.tools.protoc -I/build --python_out=/build --grpc_python_out=/build /build/keyvalue.proto
