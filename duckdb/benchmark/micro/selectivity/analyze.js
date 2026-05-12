result.results.result.map((obj) => [
  obj.dir,
  {
    sel: obj.result.raw_selectivity,
    mode: obj.result.mode,
    ...Object.fromEntries(
      Object.entries(obj.result.results).map(([method, data]) => [
        method,
        data.time[0].row_count,
      ]),
    ),
  },
]);
