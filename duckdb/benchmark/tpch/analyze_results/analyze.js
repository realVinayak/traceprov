// Just a collection of simple JS files

const math = require("mathjs");

let slice_num = 5;
const get_infer = (result) =>
  result.infer_results.map((x) => ({
    median: math.median(
      x.profile.map(({ latency }) => latency).slice(slice_num),
    ),
    stdev: math.std(x.profile.map(({ latency }) => latency).slice(slice_num)),
  }));

const augment_extra = (data) => ({
  ...data,
  base_stdev_ratio: data.base_stdev / data.base_mean,
  capture_stdev_ratio: data.capture_stdev / data.capture_mean,
});
const get_nice_data = (file) =>
  Object.fromEntries(
    Object.entries(file.results).map(([qnum, qdata]) => [
      qnum,
      augment_extra({
        base: math.median(
          qdata.result.base_profile.map((x) => x.latency).slice(slice_num),
        ),
        base_mean: math.mean(
          qdata.result.base_profile.map((x) => x.latency).slice(slice_num),
        ),
        base_stdev: math.std(
          qdata.result.base_profile.map((x) => x.latency).slice(slice_num),
        ),
        capture: math.median(
          qdata.result.capture_profile.map((x) => x.latency).slice(slice_num),
        ),
        capture_mean: math.mean(
          qdata.result.capture_profile.map((x) => x.latency).slice(slice_num),
        ),
        capture_stdev: math.std(
          qdata.result.capture_profile.map((x) => x.latency).slice(slice_num),
        ),
        backtrace: get_infer(qdata.result),
      }),
    ]),
  );

const read_json_files = (dirs) =>
  dirs.map((dir) => require(`./${dir}/result.json`));

// let files = [
//   "local_test_high_priority_optimized-y__threads-1__merge_chunks-y__table_stats-y_2026_05_04_13_58_19/",
//   "local_test_high_priority_optimized-y__threads-2__merge_chunks-y__table_stats-y_2026_05_04_13_47_56/",
//   "local_test_high_priority_optimized-y__threads-8__merge_chunks-y__table_stats-y_2026_05_04_13_56_23/",
//   "local_test_high_priority_optimized-y__threads-4__merge_chunks-y__table_stats-y_2026_05_04_13_48_44/",
// ];

// let files = [
//   "local_test_high_priority_warmup_optimized-y__threads-2__merge_chunks-y__table_stats-y_2026_05_04_15_21_52",
//   "local_test_high_priority_warmup_optimized-y__threads-4__merge_chunks-y__table_stats-y_2026_05_04_15_28_15",
//   "local_test_high_priority_warmup_optimized-y__threads-8__merge_chunks-y__table_stats-y_2026_05_04_15_36_21"
// ];

let files = [
"local_test_latest_duckdb_warmup_4_optimized-y__threads-1__merge_chunks-y_2026_05_04_17_27_29",
"local_test_latest_duckdb_warmup_4_optimized-y__threads-2__merge_chunks-y_2026_05_04_17_34_40",
"local_test_latest_duckdb_warmup_4_optimized-y__threads-4__merge_chunks-y_2026_05_04_17_38_27",
"local_test_latest_duckdb_warmup_4_optimized-y__threads-8__merge_chunks-y_2026_05_04_17_42_10"
]
let files_data = read_json_files(files);
let analyzed_data = files_data.map(get_nice_data);
