# Generate base tables
python3 create_data.py --config config_all.json -incr 24 -table 10 --db polynomial_data
# Generate a backup of the current database
pg_dump -d polynomial_data -f backup.sql
# Create empty provsql database
createdb polymial_data_provsql
# Copy data to provsql database
psql -d polynomial_data_provsql -f backup.sql
# Create provsql mapping tables
python3 create_data.py --config config_all.json -incr 24 -table 10 --no-dump_data --provsql
