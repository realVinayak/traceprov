import json
import sys
import os

def run(test_config, executable, out_file=None):
    config_file = test_config

    with open(config_file) as cf:
        config = json.loads(cf.read())

    extras = f""
    static_file = '/tmp/f.ids'
    if out_file is not None:
        extras += f" -f {static_file}"
    
    infer_sh = f'{executable}' + extras
    print(infer_sh)

    assert(os.system(infer_sh) == 0)

    print('outfile', out_file)
    if out_file is None: return

    with open(static_file) as sf:
        raw_ids = sf.read()
        ids = [[int(cell) for cell in line.split(',')] for line in raw_ids.split('\n') if len(line) > 0]

    data = {}
    for _idx, column in enumerate(config['pk_order']):
        column_contents = sorted(list(set([line[_idx] for line in ids])))
        
        data[column] = column_contents
    
    with open(out_file, 'w') as of:
        of.write(json.dumps(data, indent=4))

if __name__ == '__main__':
    run()
    

