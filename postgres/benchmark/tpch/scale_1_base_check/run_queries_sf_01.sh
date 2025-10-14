#!/bin/bash

for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22;do 
    PGPASSWORD=postgres psql -U postgres tpch_01_v01 -A --field-separator='|' -P "footer=off" -f ../scale_1/params_default/${i}/base.sql > ./result/q${i}.out;
done
