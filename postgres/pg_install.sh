#!/bin/bash

set -e
set -x
PG_PKG_LIB_DIR=$PG_PKG_LIB_DIR
if [ "$PG_PKG_LIB_DIR" = "" ]; then
PG_PKG_LIB_DIR=$(pg_config --pkglibdir)
else
PG_PKG_LIB_DIR="/usr/lib/postgresql/$PG_PKG_LIB_DIR/lib"
fi
echo "PG_LIB_DIR: $PG_PKG_LIB_DIR"
current_dir=$(pwd)
cd $1
for file in *; do echo "Mapping ${file} to ${file%%.*}$2.${file#*.}" && sudo cp "$file" "${PG_PKG_LIB_DIR}/${file%%.*}$2.${file#*.}"; done
cd $current_dir
suffix=$2
sed 's/__FILE__/libtraceprov'${suffix}'/g' traceprov.sql > traceprov_${suffix}.auto.sql
sed 's/__FILE__/libtraceprov_infer'${suffix}'/g' traceprov_return_infer.sql > traceprov_return_infer_${suffix}.auto.sql
sed 's/__FILE__/libtraceprov_infer'${suffix}'/g' traceprov_return_infer_template.sql > traceprov_return_infer_template_${suffix}.auto.sql
set +x
set +e