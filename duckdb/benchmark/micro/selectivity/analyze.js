result.results.result
  .map((obj) => [
    obj.dir,
    {
      sel: obj.result.raw_selectivity,
      mode: obj.result.mode,
      ...Object.fromEntries(
        Object.entries(obj.result.results).map(([method, data]) => [
          method,
          {
            row_count: data.time[0].row_count,
            latency: math.median(data.profile.map((x) => x.latency)),
          },
        ]),
      ),
    },
  ])
  .reduce(
    (prev, curr) => ({
      ...prev,
      [curr[1]["mode"]]: {
        ...prev[curr[1]["mode"]],
        [curr[0]]: { ...prev[curr[0]], [curr[1]["sel"]]: curr[1] },
      },
    }),
    {},
  );
