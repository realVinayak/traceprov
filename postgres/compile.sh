#!/bin/bash

cc -fPIC -c -I/usr/include/postgresql/14/server/ test_udfs.c -o test_udfs_$1.o
cc -shared test_udfs_$1.o -o test_udfs_$1.so
sudo cp test_udfs_$1.so /usr/lib/postgresql/14/lib