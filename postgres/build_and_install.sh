#!/bin/bash

# Does all the work.

set -x
set -e
autoconf
automake
current_dir=$(pwd)
# This shouldn't require root.
rm -rf bld
# bld is just a temp dir.
mkdir bld
./configure --prefix=$current_dir/bld --with-smokedduck=$2 --with-smokedduck_include=$3
make clean
make
make install
./pg_install.sh $current_dir/bld/lib/ $1

set +e
set +x