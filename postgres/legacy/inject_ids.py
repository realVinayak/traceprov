import sys
import os

def main():
    with open(sys.argv[1]) as raw_sql_f:
        raw_sql = raw_sql_f.read()
    
    with open(sys.argv[2]) as id_file:
        ids = id_file.read()


    new_sql = raw_sql.replace("%s", ids, 1)

    with open(sys.argv[3], 'w') as injected_sql_f:
        injected_sql_f.write(''.join(new_sql))

    os.system(f"cp {sys.argv[3]} /home/postgres/")
    os.system(f"chown -R postgres:postgres /home/postgres/")

if __name__ == '__main__':
    main()






