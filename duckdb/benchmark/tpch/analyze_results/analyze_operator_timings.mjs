// const { map } = require("mathjs");
import { writeFile } from "fs/promises";

import { createRequire } from "module";
import { json } from "stream/consumers";
const require = createRequire(import.meta.url);

const args = process.argv.slice(2);
const result_file = args[0];
const out_file = args[1];
console.log("reading: ", result_file);
const result = require(result_file).results;

const _get_children = (node) => {
  const next_children = node.children.reduce(
    (prev, curr) => {
      const child_result = _get_children(curr);
      return [
        [...child_result[0], ...prev[0]],
        { ...child_result[1], ...prev[1] },
      ];
    },
    [[], {}],
  );
  if (node.name == "PROJECTION") return next_children;
  return [
    [node.opid],
    {
      ...next_children[1],
      [node.opid]: next_children[0],
    },
  ];
};

const get_children = (plan) => _get_children(plan);
const get_op_type = (plan) => {
  return {
    [plan.opid]: plan.name.trim().toLowerCase(),
    ...plan.children.reduce(
      (prev, curr) => ({
        ...prev,
        ...get_op_type(curr),
      }),
      {},
    ),
  };
};

const get_nice_call_stats = (prefix, call_stats) => {
  return Object.fromEntries(
    Object.entries(call_stats ?? {}).map(([key, value]) => [
      `${prefix}_${key}`,
      value,
    ]),
  );
};

const map_result = (result) => {
  return Object.entries(result).flatMap(
    ([
      query_num,
      {
        sample_inference_result: { stats, query_stats },
      },
    ]) => {
      const main_stats = stats[0];
      const plan = JSON.parse(main_stats.plan);
      const children = get_children(plan)[1];
      const type_per_oid = get_op_type(plan);
      // console.log(query_num, children);
      // console.log(query_num, type_per_oid);
      // console.dir(plan, { depth: null });
      const mapped_query_stats = (log_value) =>
        query_stats?.map((offset) =>
          Object.fromEntries(
            offset
              .filter(({ log }) => log == log_value)
              .map(({ opid, stats, log }) => [
                opid,
                { log: log, ...JSON.parse(stats) },
              ]),
          ),
        );
      // console.log(mapped_query_stats[0]);
      const self_result = (query_stats, index) =>
        Object.entries(children).map(([parent, children]) =>
          parent in query_stats
            ? {
                offset: index,
                query_num: query_num,
                oid_id: parent,
                total_time: query_stats[parent].total_time,
                type: type_per_oid[parent],
                ...query_stats[parent],
                call_stats: null,
                ...get_nice_call_stats(
                  "call_stats",
                  query_stats[parent].call_stats,
                ),
                left_pointer_diff_stats: null,
                ...get_nice_call_stats(
                  "left_pointer_diff_stats",
                  query_stats[parent].left_pointer_diff_stats,
                ),
                right_pointer_diff_stats: null,
                ...get_nice_call_stats(
                  "right_pointer_diff_stats",
                  query_stats[parent].right_pointer_diff_stats,
                ),
                // join_diff_stats: null,
                // ...get_nice_call_stats(
                //   "join_diff_stats",
                //   query_stats[parent].join_diff_stats,
                // ),
                self_time:
                  query_stats[parent].total_time -
                  children.reduce(
                    (prev, child) =>
                      (child in query_stats
                        ? query_stats[child].total_time
                        : 0) + prev,
                    0,
                  ),
              }
            : null,
        );
      const op_timings = mapped_query_stats(0)?.flatMap(self_result);
      const log_op_timings = mapped_query_stats(1)?.flatMap(self_result);
      // console.log(op_timings);
      return [...op_timings, ...log_op_timings];
      // return self_result;
    },
  );
};

async function createJsonFile(data) {
  try {
    // The null and 2 parameters format the JSON to be human-readable
    // console.log(data);
    const jsonString = JSON.stringify(data, null, 2);
    // console.log(jsonString);
    await writeFile(out_file, jsonString, "utf8");
    console.log("JSON file successfully created!");
  } catch (error) {
    console.error("Error writing file:", error);
  }
}

createJsonFile(map_result(result).filter((x) => x != null));
