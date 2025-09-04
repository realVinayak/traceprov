from collections import defaultdict
import sys
import json
import os
import time
import re

TOOK_TIME_RE = r'Took: (\d+)'

from capture import run_validate

def safe_run(cmd):
    assert(os.system(cmd) == 0)

THROWAWAY = 3

def run():

    config_file = sys.argv[1]

    validate_config_dirs = None
    executable = None

    if len(sys.argv) > 2:
        validate_config_dirs = sys.argv[2]
        executable = sys.argv[3]


    with open(config_file) as cf:
        config = json.loads(cf.read())

    test_queries = config['queries']
    repeat = config['repeat'] + THROWAWAY
    test_dirs = config['subdirs']
    db_name = config['db_name']
    validate = config.get('validate', False)


    baseline_result_store = defaultdict(lambda: defaultdict(list))
    traceprov_result_store = defaultdict(lambda: defaultdict(list))

    def _add_duration(test_dir, query, duration, previous):
        return  {
                    **previous, 
                    test_dir: {
                        **previous.get(test_dir, {}),
                        query: [*previous.get(test_dir, {}).get(query, []), duration]
                        }
                }

    for test_dir in test_dirs:
        path = '/'.join([*config_file.split('/')[:-1], test_dir, ''])


        for query in test_queries:

            for repetition in range(repeat):
                print("[baseline] Running query:\t", query, 'num', repetition, 'dir:\t', test_dir)
                start = time.perf_counter()
                safe_run(f"PGPASSWORD=postgres psql -U postgres {db_name} -f {path}base/{query}.sql > /dev/null")
                end = time.perf_counter()

                duration = end - start

                # if we're going to throwaway, don't bother storing it
                if repetition < THROWAWAY: continue

                baseline_result_store = _add_duration(test_dir, query, duration, baseline_result_store)

            for repetition in range(repeat):
                print('[traceprov] Running query:\t', query, 'num', repetition, 'dir:\t', test_dir)
                safe_run(f'echo "select reinit_state();" | PGPASSWORD=postgres psql -U postgres {db_name}')
                start = time.perf_counter()
                safe_run(f"PGPASSWORD=postgres psql -U postgres {db_name} -f {path}traceprov/{query}.traceprov.sql > /dev/null")
                end = time.perf_counter()

                duration = end - start

                if repetition < THROWAWAY: continue

                # Now, we'd validate it.
                assert validate_config_dirs

                steps = config.get('validate_steps')
                query_steps = steps.get(str(query), [None])

                time_taken = 0

                for step in query_steps:

                    if step is None:
                        validate_file_sql = f"{path}/validate/{query}.validate.sql"
                        validate_config = f"{validate_config_dirs}/{query}.config.json"
                    else:
                        validate_file_sql = f"{path}/validate/{query}.{step}.validate.sql"
                        validate_config = f"{validate_config_dirs}/{query}.{step}.config.json"

                    safe_run(("rm -f ./temp.out")) 
                    resp = run_validate(
                        validate_config,
                        validate_file_sql,
                        db_name,
                        raw_sql_file=f"{path}base/{query}.sql",
                        executable=executable,
                        out_file='./temp.out',
                        print_stmts=True,
                        diff_out=False,
                        dry_run=(not validate) or (query in config.get('id_only', []))
                    )
                    assert((resp in [0, -1]) or str(query) in config.get('expected_diff_failures'))

                    with open("./temp.out") as to:
                        temp_out = to.read()

                    time_taken += int(re.search(TOOK_TIME_RE, temp_out).groups()[0])

                traceprov_result_store = _add_duration(test_dir, query, (duration, time_taken), traceprov_result_store)


    with open('baseline_result_store.json', 'w') as bsf:
        bsf.write(json.dumps(baseline_result_store))
    
    with open('traceprov_result_store.json', 'w') as trs:
        trs.write(json.dumps(traceprov_result_store))

    print(baseline_result_store)
    print(traceprov_result_store)
            


    
if __name__ == '__main__':
    run()


