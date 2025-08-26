import sys
import os
import re
import json


INFER_RE = r"""([^"]+)" <@ '{((?:\d+|,\d+)+)}"""

def infer_ranges():

    in_sql_filepath = sys.argv[1]
    outpath = None if len(sys.argv) < 3 else sys.argv[2]

    with open(in_sql_filepath) as isf:
        in_sql = isf.read()

    matches = re.findall(INFER_RE, in_sql)

    match_data = {key: ids for (key, ids) in matches}

    with open(outpath, 'w') as op:
        op.write(json.dumps(match_data, indent=4))



if __name__ == '__main__':
    infer_ranges()
    
    
