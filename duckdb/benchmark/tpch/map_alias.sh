# process.sh
file="$1"
set -e
set -x

python3 map_alias.py --file "$file"
npx sql-formatter -l duckdb --fix "$file"
