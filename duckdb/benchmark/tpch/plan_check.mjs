import { createRequire } from "module";
import { json } from "stream/consumers";
const require = createRequire(import.meta.url);

const args = process.argv.slice(2);
const file_1 = args[0];
const file_2 = args[1];

const reduce_plan = (plan) => {
  const keys = Object.fromEntries(
    Object.entries(plan).filter(([key]) => key != "extra"),
  );
  return {
    ...keys,
    children: plan.children.map(reduce_plan),
  };
};
function getFileMap(result_file) {
  const result = require(result_file).results;
  const qmapped = Object.fromEntries(
    Object.entries(result).map(([qnum, qresult]) => [
      qnum,
      JSON.stringify(
        reduce_plan(JSON.parse(qresult.result.sd.capture_stats[0].plan)),
      ),
    ]),
  );
  return qmapped;
}

const plan_map_1 = getFileMap(file_1);
const plan_map_2 = getFileMap(file_2);

function getDiff(plan1, plan2, msg) {
  Object.keys(plan1).forEach((key) => {
    console.log(msg, key);
    if (plan2[key] === undefined) return;
    if (plan1[key] != plan2[key]) console.log("got diff at ", key);
    console.log(plan1[key]);
    console.log(plan2[key]);
  });
}

getDiff(plan_map_1, plan_map_2, "1->2");
getDiff(plan_map_2, plan_map_1, "2->1");
