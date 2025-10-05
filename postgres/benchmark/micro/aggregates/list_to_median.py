import statistics
import json
import sys
def compute_average(in_dict):
 
    return {key: statistics.mean(value or [0]) if isinstance(value, list) else (compute_average(value) if isinstance(value, dict) else value )for key, value in in_dict.items()}

def main():
    file_name = sys.argv[1]
    with open(file_name) as f:
        dict_parse = json.loads(f.read())
    computed = compute_average(dict_parse)
    print(json.dumps(computed, indent=4))

if __name__ == '__main__':
    main()
