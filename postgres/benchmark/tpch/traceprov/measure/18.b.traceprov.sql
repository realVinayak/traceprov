SELECT *, mark_later(mapped_agg) FROM (
    SELECT
        l_orderkey,
        agg_map_parallel(l_orderkey, l_linenumber) as mapped_agg
    FROM     lineitem
    GROUP BY l_orderkey
) HAVING   SUM(l_quantity) > 300;