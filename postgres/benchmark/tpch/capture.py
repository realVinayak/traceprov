import json
import sys
import os

DEFAULT_ID_TABLE_NAME = "captured_id"

def make_select(table, keys):
    return ','.join([f"{table}.{key}" for key in keys])

def make_table(table, keys):
    columns = ',\n'.join([f"{pk_name} INTEGER" for pk_name in keys])
    create_table_sql = '\n'.join([
        f'CREATE TABLE {table} (',
        columns,
        f");"
    ])
    return create_table_sql

def run_validate(
        config_file_name, 
        validate_sql_file, 
        db_name, 
        raw_sql_file=None, 
        executable="./traceprov_infer.o", 
        out_file=None, 
        print_stmts=True,
        dry_run=False,
        diff_out=True
    ):
    with open(config_file_name) as cf:
        config = json.loads(cf.read())


    id_file_name = config.get('id_file')
    layer_number = config.get('layer_number', 1)
    subq_table = config.get('subq_table_name')
    ignore_group = config.get('ignore_gn')

    extras = ""
    if subq_table:
        extras += f" -s.layer_num {config.get('subq_layer')}"
        extras += f" -s.out {config.get('subq_id_file')}"

    if ignore_group is not None:
        extras += f" -ig {int(ignore_group)}"
    
    if id_file_name and not dry_run:
        extras += f" -f {id_file_name}"

    infer_sh = f'{executable} -l {layer_number}' + extras
    if out_file:
        infer_sh += f" > {out_file}"

    print(infer_sh)
    assert(os.system(infer_sh)== 0)

    if id_file_name is None or dry_run:
        return -1

    if subq_table:
        assert(os.system(f"rm -f /home/postgres/{config.get('subq_id_file')}") == 0)

    assert(os.system(f'rm -f /home/postgres/{id_file_name}') == 0)

    with open(f'/home/postgres/{id_file_name}', 'w') as f:
        with open(id_file_name) as idf:
            f.write(idf.read())

    if config.get('subq_id_file'):
        with open(f"/home/postgres/{config.get('subq_id_file')}", 'w') as f:
            with open(config.get('subq_id_file')) as idf:
                f.write(idf.read())


    assert(os.system(f"chown -R postgres:postgres /home/postgres") == 0)

    ID_TABLE_NAME = config.get('id_table_name', DEFAULT_ID_TABLE_NAME)

    create_table_sql = make_table(
        ID_TABLE_NAME,
        config['pk_order']
    )

    copy_from_file_sql = f"COPY {ID_TABLE_NAME} FROM '/home/postgres/{id_file_name}' WITH (FORMAT csv, DELIMITER ',');"

    with open(validate_sql_file) as vsf:
        sql_to_inject = vsf.read()

    for participating in config['inserts']:
        ref = participating['ref']
        keys = participating['keys']
        is_subq = participating.get('is_sub', False)

        inner_table = subq_table if is_subq else ID_TABLE_NAME

        key_select_sql = f"SELECT {make_select(inner_table, keys)} FROM {inner_table}"
        sql_to_inject = sql_to_inject.replace(f"%{ref}%", key_select_sql)
    
    final_sql = [
        f"DROP TABLE IF EXISTS {ID_TABLE_NAME};",
        create_table_sql,
        copy_from_file_sql,
        '-- SQL --',
    ]

    if subq_table:
        final_sql.append(f"DROP TABLE IF EXISTS {subq_table};")
        subq_id_file = config['subq_id_file']
        subq_create_table = make_table(
            subq_table,
            config['subq_pk_order']
        )
        final_sql.append(subq_create_table)
        final_sql.append(f"COPY {subq_table} FROM '/home/postgres/{subq_id_file}' WITH (FORMAT csv, DELIMITER ',');")

    with open("/tmp/create_id_table.sql", 'w') as f:
        f.write('\n'.join(final_sql))

    with open("/tmp/injected.sql", 'w') as f:
        f.write((sql_to_inject))

    setup_cmd = f'PGPASSWORD=postgres psql -U postgres {db_name} -f /tmp/create_id_table.sql'
    if print_stmts: 
        print(setup_cmd)
    else:
        setup_cmd += " > /dev/null"
    assert(os.system(setup_cmd) == 0)

    if raw_sql_file is None: return -1
    #  PGPASSWORD=postgres psql -U postgres tpch_01_v01 -A --field-separator='|' -P "footer=off" -f ../../raw_queries/1.sql > ../result/q1.out
    get_run_cmd = lambda in_file, out_file: f"PGPASSWORD=postgres psql -U postgres {db_name} -A --field-separator='|' -P \"footer=off\" -f {in_file} > {out_file}"
    
    injected_run_cmd = get_run_cmd('/tmp/injected.sql', '/tmp/injected.out')
    if print_stmts: 
        print(injected_run_cmd)

    assert(os.system(injected_run_cmd) == 0)

    raw_run_cmd = get_run_cmd(raw_sql_file, '/tmp/raw.out')
    if print_stmts: 
        print(raw_run_cmd)

    assert(os.system(raw_run_cmd) == 0)

    diff_return = os.system('diff -u /tmp/injected.out /tmp/raw.out' + (" > /dev/null" if not diff_out else ""))
    return diff_return

def capture():

    config_file_name = sys.argv[1]
    validate_sql_file = sys.argv[2]
    db_name = sys.argv[3]

    if len(sys.argv) == 5:
        raw_sql_file = sys.argv[4]
    else:
        raw_sql_file = None

    run_validate(config_file_name, validate_sql_file, db_name, raw_sql_file)

if __name__ == '__main__':
    capture()



