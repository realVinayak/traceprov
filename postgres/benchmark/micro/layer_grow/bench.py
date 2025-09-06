import sys
import os
import re
import statistics
import math

import matplotlib.pyplot as plt

TIME_RE = r'Took: (\d*).(\d*)'

def parse_time(file):
    with open(file) as f:
        contents = f.read()
    
    matches = re.search(TIME_RE, contents).groups()
    first = matches[0]
    second = matches[1]
    parsed_time = f'{first}.{second}'
    return float(parsed_time)

def safe_run(cmd):
    print(cmd)
    assert(os.system(cmd) == 0)

ranges = [
    (20, "1 MB"),
    (21, "2 MB"),
    (22, "4 MB"),
    (23, "8 MB"),
    (24, "16 MB"),
    (25, "32 MB"),
    (26, "64 MB"),
    (27, "128 MB"),
    (28, "256 MB"),
    (29, "512 MB"),
    (30, "1 GB"),
    (31, "2 GB"),
    (32, "4 GB"),
]

MAX = 13
START = 20
THROWAWAY = 5
REPEAT = 10

PAGE_INCREMENTS = [
    ((1 << 5), ("128 KB")),
    # ((1 << 6), ("256 KB")),
    ((1 << 7), ("512 KB")),
    # ((1 << 8), ("1 MB")),
    ((1 << 9), ("2 MB")),
    # ((1 << 10), ("4 MB")),
    ((1 << 11),  ("8 MB")),
    # ((1 << 12),  ("16 MB")),
    ((1 << 13),  ("32 MB")),
    # ((1 << 14), ("64 MB")),
    ((1 << 15), ("128 MB")),
    # ((1 << 16), ("256 MB")),
    ((1 << 17), ("512 MB")),
    ((1 << 18), ("1 GB")),
]


def run():
    computed_ranges = [START + idx for idx in range(MAX)]
    assert computed_ranges == [val[0] for val in ranges]
    
    labels = [val[1] for val in ranges]
    assert len(labels) == len(set(labels))

    safe_run("make clean")

    result = {}
    total_repeats = THROWAWAY + REPEAT
    for (increment, incr_label) in PAGE_INCREMENTS:
        outer_key = (increment, incr_label)
        if outer_key not in result:
            result[outer_key] = {}
        
        safe_run(f"make run inc={increment}")
        current_result = result[outer_key]
        for (size, label) in ranges:
            inner_key = (size, label)
            if inner_key not in current_result:
                current_result[inner_key] = []
            
            for repeat in range(total_repeats):
                print(f"ON: ({(size, label, repeat, incr_label)})")
                safe_run(f"./run.o {size} > ./tmp.out")

                if repeat < THROWAWAY: continue

                current_result[inner_key].append(parse_time("./tmp.out"))
    
    return (result)
        
            
def compute_median(result):

    flattened = {
        outer: {
            inner: statistics.median(times)
            for inner, times in per_incr.items()
        }
        for (outer, per_incr) in result.items()
    }


    return (flattened)

def flatten_per_incr(per_incr):
    result = []
    for r in ranges:
        result.append(per_incr[r])
    return result

if __name__ == '__main__':

    medianed = compute_median(run())

    with open("result.txt", 'w') as f:
        f.write(repr(medianed))

    final_sizes = [(r[0]) for r in ranges]
    final_times = {
        outer: flatten_per_incr(per_incr)
        for (outer, per_incr) in medianed.items()
    }

    fig, ax = plt.subplots()
    
    for idx, (label, final_time) in enumerate(sorted(final_times.items(), key=lambda x: x[0][0])):
        if idx < 3:
            ax.plot(final_sizes, final_time, 'o-', label=label[1])
            for _st_idx, (size, time) in enumerate(zip(final_sizes, final_time)):
                if _st_idx < len(final_sizes) - 3:
                    continue
                ax.annotate(
                    f"{time:.3f}",
                    xy=(size*(0.975), time*(1.01)),
                    fontsize=6,
                    ha='left'
                )
        else:
            ax.plot(final_sizes, final_time, label=label[1])
    
    # plt.yscale('log', base=10)
    plt.ylabel("Time (s)")
    plt.legend()


    ax.set_xticks(final_sizes)
    ax.set_xticklabels([(r[1]) for r in ranges], rotation=30)
    ax.minorticks_on()
    ax.set_xlabel(("Final size"))
    plt.subplots_adjust(bottom=0.17)
    plt.savefig("final_size_time.png")
