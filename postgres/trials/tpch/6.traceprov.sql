SELECT *, mark_later(mapped_agg) FROM (SELECT 
    SUM(l_extendedprice * l_discount)  AS revenue, 
    agg_map_parallel(l_orderkey, l_linenumber) as mapped_agg
FROM   lineitem
WHERE  l_shipdate >= DATE '1997-01-01'
AND    l_shipdate <  DATE '1997-01-01' + interval '1' year
AND    l_discount BETWEEN 0.05         - 0.01 AND    0.05 + 0.01
AND    l_quantity < 25 limit 1) f;