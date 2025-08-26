import sys
import os
import json
from os import listdir
from os.path import isfile, join

def get_files(dir_name):
    return [f for f in listdir(dir_name) if isfile(join(dir_name, f))]

def get_computed(base_range):
    num = base_range.split('.')[0]
    computed_range_fn = f"{num}.ranges"
    return computed_range_fn
    
def check_ranges_equivalent(base, expected):
    is_equivalent = True
    for expected_value in expected:
        is_found = False
        for base_idx, base_value in enumerate(base[:-1]):
            lower_value = base_value
            upper_value = base[base_idx+1]

            if lower_value <= expected_value and expected_value < upper_value:
                is_found = True
                break
        
        if not is_found:
            print("didn't find for",expected_value)
        is_equivalent = is_equivalent and is_found
    return is_equivalent

def check_ranges(base_dir, computed_dir):
    base_ranges = get_files(base_dir)
    computed_ranges = get_files(computed_dir)

    assert(len(base_ranges) == len(computed_ranges))

    for base_range in base_ranges:
        computed_range = get_computed(base_range)

        print("checking", base_range, computed_range)

        with open(join(base_dir, base_range)) as f:
            base_result = json.loads(f.read())

        with open(join(computed_dir, computed_range)) as f:
            computed_result = json.loads(f.read())
        
        assert(len(base_result) == len(computed_result))

        for base_key in base_result:
            base_ranges_for_key = [int(key) for key in base_result[base_key].split(',')]
            computed_ranges_for_key = computed_result[base_key]
            print(base_key, check_ranges_equivalent(base_ranges_for_key, computed_ranges_for_key))


if __name__ == '__main__':
    check_ranges(sys.argv[1], sys.argv[2])
