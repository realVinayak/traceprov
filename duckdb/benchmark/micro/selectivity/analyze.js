const math = require("mathjs");

const macos_gprom_optimized_files = [
  "macos_gprom_optimized-y__threads-1_2026_05_12_11_12_01",
  "macos_gprom_optimized-y__threads-2_2026_05_12_11_57_10",
  "macos_gprom_optimized-y__threads-4_2026_05_12_12_23_20",
  "macos_gprom_optimized-y__threads-8_2026_05_12_12_39_12",
  "macos_gprom_optimized-y__threads-10_2026_05_12_12_54_03",
  "macos_gprom_optimized-y__threads-12_2026_05_12_13_08_24",
];

const macos_gprom_non_optimized_files = [
  "macos_gprom_optimized-n__threads-1_2026_05_12_14_23_54",
  "macos_gprom_optimized-n__threads-2_2026_05_12_15_08_43",
  "macos_gprom_optimized-n__threads-4_2026_05_12_15_34_32",
  "macos_gprom_optimized-n__threads-8_2026_05_12_15_49_59",
  "macos_gprom_optimized-n__threads-10_2026_05_12_16_04_26",
  "macos_gprom_optimized-n__threads-12_2026_05_12_16_18_25",
];

const macos_gprom_optimized = macos_gprom_optimized_files.map((file) =>
  require(`./results/${file}/result.json`),
);
const macos_gprom_non_optimized = macos_gprom_optimized_files.map((file) =>
  require(`./results/${file}/result.json`),
);

const reduce_data = (result) =>
  result.reduce(
    (prev, curr) => ({
      ...prev,
      [curr[1]["mode"]]: {
        ...(prev[curr[1]["mode"]] ?? {}),
        [curr[0]]: {
          ...((prev[curr[1]["mode"]] ?? {})[curr[0]] ?? {}),
          [curr[1]["sel"]]: curr[1],
        },
      },
    }),
    {},
  );

const map_reduce_result = (result) => {
  return reduce_data(
    result.results.result.map((obj) => [
      obj.dir,
      {
        sel: obj.result.raw_selectivity,
        mode: obj.result.mode,
        ...Object.fromEntries(
          Object.entries(obj.result.results).map(([method, data]) => [
            method,
            {
              row_count: data.time[0].row_count,
              latency: math.median(data.profile.slice(5).map((x) => x.latency)),
              latency_stdev:
                math.std(data.profile.slice(5).map((x) => x.latency)) /
                math.mean(data.profile.slice(5).map((x) => x.latency)),
            },
          ]),
        ),
      },
    ]),
  );
};

macos_gprom_optimized.map((res, idx) => {
  console.log("Result pair: ", macos_gprom_optimized_files[idx]);
  console.dir(map_reduce_result(res), { depth: null });
});
