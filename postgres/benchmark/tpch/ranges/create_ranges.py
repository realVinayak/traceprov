import sys
import re

ID_RE = r"binary_search_array_pos\('{((?:\d+|,\d+)+)}'"
COLUMNS_RE = r"binary_search_array_pos\('{.*}', .*\"(.*)\"\)"

def create_ranges():
    in_sql_path = sys.argv[1]
    template = sys.argv[2] if len(sys.argv) == 3 else None

    with open(in_sql_path) as isp:
        input_sql = isp.read()
    
    id_groups = re.findall(ID_RE, input_sql)
    # print(id_groups)

    columns = re.findall(COLUMNS_RE, input_sql)
    print("--", columns)

    if template is None: return

    assert len(id_groups) == len(columns)

    agg_map_input = [
        f"custom_binary_search_value(ARRAY [{_ids}], {column_key})" 
        for _ids, column_key in zip(id_groups, columns)
    ]

    with open(template) as template_file:
        template_sql = template_file.read()
    
    template_sql = template_sql.replace('%0%', "\n" + ',\n'.join(agg_map_input) + "\n")
    print(template_sql)


if __name__ == '__main__':
    create_ranges()