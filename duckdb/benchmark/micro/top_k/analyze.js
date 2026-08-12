results.results.result
  .map((x) => [
    x.dir,
    x.result.raw_top_k_limit,
    Object.fromEntries(
      Object.entries(x.result.results).map(([method, met_res]) => [
        method,
        met_res.profile.map((et) => ({
          ...et,
          query_name: et.query_name.replaceAll("\n", "").replaceAll('"', ""),
        })),
      ]),
    ),
  ])
  .reduce(
    (prev, curr) => ({
      ...prev,
      [curr[0]]: { ...prev[curr[0]], [curr[1]]: curr[2] },
    }),
    {},
  );
