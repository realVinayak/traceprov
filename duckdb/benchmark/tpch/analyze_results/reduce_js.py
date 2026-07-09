from pathlib import Path
import sys

import json
from traceprovpy.tools.file_utils import json_read_file, just_write

def remove_keys(obj, keys):
    return {key: value for (key, value) in obj.items() if key not in keys}

def get_key(obj, keys):
    return {key: value for (key, value) in obj.items() if key in keys}

def reduce_result(obj):
    sample_result = obj['sample_inference_result']
    sample_result = remove_keys(sample_result, ['settings'])
    sample_result['profile'] = list(map(lambda x: get_key(x, 'result'), sample_result['profile']))
    return dict(sample=sample_result)

def reduce_js(in_file: Path):
    obj = json_read_file(in_file)
    _reduce_result = obj['results']
    data = {q: reduce_result(qdata) for (q, qdata) in _reduce_result.items()}
    just_write("reduced_result.json", json.dumps(data))


if __name__ == "__main__":
    reduce_js(sys.argv[1])
