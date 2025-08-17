import json
import sys
import os

ID_TABLE_NAME = "captured_id"

def make_select(table, keys):
    return ','.join([f"{table}.{key}" for key in keys])

def capture():

    id_file_name = sys.argv[1]

    assert(os.system(f'./infer.o {id_file_name}') == 0)


    config_file_name = sys.argv[2]
    validate_sql_file = sys.argv[3]
    db_name = sys.argv[4]

    if len(sys.argv) == 6:
        raw_sql_file = sys.argv[5]
    else:
        raw_sql_file = None

    assert(os.system(f'rm -f /home/postgres/{id_file_name}') == 0)

    with open(f'/home/postgres/{id_file_name}', 'w') as f:
        with open(id_file_name) as idf:
            f.write(idf.read())

    assert(os.system(f"chown -R postgres:postgres /home/postgres") == 0)

    with open(config_file_name) as cf:
        config = json.loads(cf.read())

    columns = ',\n'.join([f"{pk_name} INTEGER" for pk_name in config['pk_order']])

    create_table_sql = '\n'.join([
        f'CREATE TABLE {ID_TABLE_NAME} (',
        columns,
        f");"
    ])

    copy_from_file_sql = f"COPY {ID_TABLE_NAME} FROM '/home/postgres/{id_file_name}' WITH (FORMAT csv, DELIMITER ',');"

    with open(validate_sql_file) as vsf:
        sql_to_inject = vsf.read()

    for participating in config['inserts']:
        ref = participating['ref']
        keys = participating['keys']

        key_select_sql = f"SELECT {make_select(ID_TABLE_NAME, keys)} FROM {ID_TABLE_NAME}"
        sql_to_inject = sql_to_inject.replace(f"%{ref}%", key_select_sql)
    
    final_sql = [
        f"DROP TABLE IF EXISTS {ID_TABLE_NAME};",
        create_table_sql,
        copy_from_file_sql,
        '-- SQL --',
    ]

    with open("/tmp/create_id_table.sql", 'w') as f:
        f.write('\n'.join(final_sql))

    with open("/tmp/injected.sql", 'w') as f:
        f.write((sql_to_inject))

    setup_cmd = f'PGPASSWORD=postgres psql -U postgres {db_name} -f /tmp/create_id_table.sql'
    print(setup_cmd)
    assert(os.system(setup_cmd) == 0)

    if raw_sql_file is None: return
    #  PGPASSWORD=postgres psql -U postgres tpch_01_v01 -A --field-separator='|' -P "footer=off" -f ../../raw_queries/1.sql > ../result/q1.out
    get_run_cmd = lambda in_file, out_file: f"PGPASSWORD=postgres psql -U postgres {db_name} -A --field-separator='|' -P \"footer=off\" -f {in_file} > {out_file}"
    
    injected_run_cmd = get_run_cmd('/tmp/injected.sql', '/tmp/injected.out')
    print(injected_run_cmd)
    assert(os.system(injected_run_cmd) == 0)

    raw_run_cmd = get_run_cmd(raw_sql_file, '/tmp/raw.out')
    print(raw_run_cmd)
    assert(os.system(raw_run_cmd) == 0)

    os.system('diff /tmp/injected.out /tmp/raw.out')

if __name__ == '__main__':
    capture()



