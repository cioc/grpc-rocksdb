grpc-rocksdb
============

[![Build Status](https://travis-ci.org/cioc/grpc-rocksdb.svg?branch=master)](https://travis-ci.org/cioc/grpc-rocksdb)

grpc-rocksdb is a grpc (http://www.grpc.io/) wrapper around rocksdb (http://rocksdb.org/).  The program is built and runs within a docker container.   It is intended to be used as a component within a kubernetes pod, see kube directory for a very basic pod declaration.  I like the pod model because I can construct different parts of my system in different dependency closures (containers).  I intend to use this as a backing store for some projects in replication.   

The service definition can be found here: https://github.com/cioc/grpc-rocksdb-proto  The interface is a basic key-value storewith optimistic concurrency control.   The interface will likely evolve.  

The container is on docker hub: https://hub.docker.com/r/cioc/grpc-rocksdb/  This image is the server docker file.  There is another docker file within the tst directory.  It's a python-based test to run against the server container.

build &amp; test
================

To build the containers and run the tests:

```
./build.sh
```

To build just the main container:

```
./build.sh build
```

To just build just the test container:

```
./build.sh build-test
```

To just run the tests:

```
./build.sh test
```

To clean:

```
./build.sh clean
```

Command to be run within the container:

```
./build/grpc-rocksdb <addr> <storage path>
```



