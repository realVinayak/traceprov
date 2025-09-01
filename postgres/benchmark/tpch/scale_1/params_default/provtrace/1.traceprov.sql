-- using default substitutions

SELECT *, mark_later(mapped_agg)
FROM  (SELECT
  l_returnflag,
  l_linestatus,
  sum(l_quantity) AS sum_qty,
  sum(l_extendedprice) AS sum_base_price,
  sum(l_extendedprice * (1 - l_discount)) AS sum_disc_price,
  sum(l_extendedprice * (1 - l_discount) * (1 + l_tax)) AS sum_charge,
  avg(l_quantity) AS avg_qty,
  avg(l_extendedprice) AS avg_price,
  avg(l_discount) AS avg_disc,
  count(*) AS count_order,
  traceprov_agg_key_parallel(
    1,
    l_orderkey,
    l_linenumber
  ) as mapped_agg
FROM
  lineitem
WHERE
  l_shipdate <= date '1998-12-01' - INTERVAL '90' DAY
GROUP BY
  l_returnflag,
  l_linestatus
  ) f
ORDER BY
  l_returnflag,
  l_linestatus;