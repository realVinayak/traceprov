#!/bin/bash

sudo -s <<EOF
cp /var/lib/postgresql/14/main/traceprov/worker_* $1
EOF