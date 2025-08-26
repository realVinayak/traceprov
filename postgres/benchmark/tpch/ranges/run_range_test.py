import sys
import os
import json
import time

from parse_ranges import run as parse_range_run

def safe_run(cmd):
    assert(os.system(cmd) == 0)

def run_range_test():
    
    test_config_file = sys.argv[1]
    infer_executable = sys.argv[2]
    base_dir = sys.argv[3]

    with open(test_config_file) as tcf:
        test_config = json.loads(tcf.read())
    
    queries = test_config['queries']
    db_name = test_config['db_name']

    get_path = lambda ready_dir: '/'.join([*test_config_file.split('/')[:-1], *ready_dir, ''])

    traceprov_dir = get_path(['traceprov_ready'])

    for query in queries:
        query_path = f"{traceprov_dir}/{query}.sql"
        test_range_config = f"infer_configs/{query}.range.config.json"
        out_file = f"{get_path(['traceprov_result'])}/{query}.ranges"
    
        print(query_path)

        safe_run(f'echo "select reinit_state(0);" | PGPASSWORD=postgres psql -U postgres {db_name}')
        safe_run(f"PGPASSWORD=postgres psql -U postgres {db_name} -f {query_path} > /dev/null")

        parse_range_run(test_range_config, infer_executable, out_file)



        



if __name__ == '__main__':
    run_range_test()
        

