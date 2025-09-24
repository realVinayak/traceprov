#!/bin/bash

python3 prepare_data.py -skew 1 -num 1_000_000 -db microbench_agg_02 -g 10 --drop
python3 prepare_data.py -skew 1 -num 5_000_000 -db microbench_agg_02 -g 10
python3 prepare_data.py -skew 1 -num 10_000_000 -db microbench_agg_02 -g 10
python3 prepare_data.py -skew 1 -num 50_000_000 -db microbench_agg_02 -g 10
python3 prepare_data.py -skew 1 -num 100_000_000 -db microbench_agg_02 -g 10

python3 prepare_data.py -skew 1 -num 1_000_000 -db microbench_agg_02 -g 100 --drop
python3 prepare_data.py -skew 1 -num 5_000_000 -db microbench_agg_02 -g 100
python3 prepare_data.py -skew 1 -num 10_000_000 -db microbench_agg_02 -g 100
python3 prepare_data.py -skew 1 -num 50_000_000 -db microbench_agg_02 -g 100
python3 prepare_data.py -skew 1 -num 100_000_000 -db microbench_agg_02 -g 100

python3 prepare_data.py -skew 1 -num 1_000_000 -db microbench_agg_02 -g 1000 --drop
python3 prepare_data.py -skew 1 -num 5_000_000 -db microbench_agg_02 -g 1000
python3 prepare_data.py -skew 1 -num 10_000_000 -db microbench_agg_02 -g 1000
python3 prepare_data.py -skew 1 -num 50_000_000 -db microbench_agg_02 -g 1000
python3 prepare_data.py -skew 1 -num 100_000_000 -db microbench_agg_02 -g 1000