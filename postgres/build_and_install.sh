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

# Copy over smokedduck too.
# this is done so that all benches get a different exectuble.
# Plus, the bld dir gets removed each time, so this guarantees we didn't
# accidentally take an older version of executable.
if [ "$2" != "" ]; then
echo "copying over smokedduck"
cp $current_dir/bld/bin/run_smokedduck $current_dir/bld/bin/run_smokedduck_$1
else
echo "copying over smokedduck not needed"
fi

set +e
set +x