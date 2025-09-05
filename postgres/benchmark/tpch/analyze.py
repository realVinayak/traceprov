import sys
import statistics
import json

def dump_csv():

    index = int(sys.argv[1])
    variance = False
    mean = False

    baseline_files = []
    traceprov_files = []
    for idx, i in enumerate(sys.argv[1:], start=1):
        if i == "-v":
            variance = True
        if i == '-m':
            mean = True
        if i == '-base':
            baseline_files.append(sys.argv[idx+1])
        if i == '-trace':
            traceprov_files.append(sys.argv[idx+1])

    baseline = {}
    traceprov = {}

    for bf in baseline_files:
        with open(bf) as bf_file:
            baseline = {**baseline, **json.loads(bf_file.read())}

    for bf in traceprov_files:
        with open(bf) as bf_file:
            traceprov = {**traceprov, **json.loads(bf_file.read())}

    baseline_data = {}
    traceprov_data = {}
    traceprov_inference = {}
    traceprov_total = {}


    for per_dir in baseline.values():
        for query_key, query_timings in per_dir.items():
            baseline_data[query_key] = [
                *baseline_data.get(query_key, []),
                *query_timings
                ]

    for per_dir_traceprov in traceprov.values():
        for query_key, query_timings in per_dir_traceprov.items():
            traceprov_data[query_key] = [
                *traceprov_data.get(query_key, []),
                *[qt[0] for qt in query_timings]
            ]
            traceprov_inference[query_key] = [
                *traceprov_inference.get(query_key, []),
                *[(qt[1] / 1000) for qt in query_timings]
            ]
            traceprov_total[query_key] = [
                *traceprov_total.get(query_key, []),
                *[qt[0] + (qt[1] / 1000) for qt in query_timings]
            ]
    
    medianed_baseline_data = {key: statistics.median(value) for (key, value) in baseline_data.items()}
    medianed_traceprov_data = {key: statistics.median(value) for (key, value) in traceprov_data.items()}
    medianed_traceprov_inference_data = {key: statistics.median(value) for (key, value) in traceprov_inference.items()}
    medianed_traceprov_total = {key: statistics.median(value) for (key, value) in traceprov_total.items()}

    mean_baseline_data = {key: statistics.mean(value) for (key, value) in baseline_data.items()}
    mean_traceprov_data = {key: statistics.mean(value) for (key, value) in traceprov_data.items()}
    mean_traceprov_inference_data = {key: statistics.mean(value) for (key, value) in traceprov_inference.items()}
    mean_traceprov_total = {key: statistics.mean(value) for (key, value) in traceprov_total.items()}

    variance_baseline_data = {key: statistics.pvariance(value) for (key, value) in baseline_data.items()}
    variance_traceprov_data = {key: statistics.pvariance(value) for (key, value) in traceprov_data.items()}
    variance_traceprov_inference_data = {key: statistics.pvariance(value) for (key, value) in traceprov_inference.items()}
    variance_traceprov_total = {key: statistics.pvariance(value) for (key, value) in traceprov_total.items()}

    # print(medianed_baseline_data)
    # print(medianed_traceprov_data)
    # print(medianed_traceprov_inference_data)
    # print(medianed_traceprov_total)

    assert(len(
        set([
            len(medianed_baseline_data), 
            len(medianed_traceprov_data), 
            len(medianed_traceprov_inference_data), 
            len(medianed_traceprov_total)
            ]
        )) == 1)
    
    get_source = lambda median, _mean, _variance: _variance if variance else (_mean if mean else median)

    for query in sorted(medianed_baseline_data, key=lambda x: int(x)):
        if index == 0: print(query)
        if index == 1: print(get_source(medianed_baseline_data, mean_baseline_data, variance_baseline_data)[query])
        if index == 2: print(get_source(medianed_traceprov_data, mean_traceprov_data, variance_traceprov_data)[query])
        if index == 3: print(get_source(medianed_traceprov_inference_data, mean_traceprov_inference_data, variance_traceprov_inference_data)[query])
        if index == 4: print(get_source(medianed_traceprov_total, mean_traceprov_total, variance_traceprov_total)[query])
        # print(
        #     query,
        #     medianed_baseline_data[query], 
        #     medianed_traceprov_data[query], 
        #     medianed_traceprov_inference_data[query],
        #     medianed_traceprov_total[query],
        #     sep=","
        # )
if __name__ == '__main__':
    dump_csv()