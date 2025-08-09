#!/bin/bash

sudo cp /etc/postgresql/14/main/postgresql.conf.sequential /etc/postgresql/14/main/postgresql.conf
sudo systemctl restart postgresql.service