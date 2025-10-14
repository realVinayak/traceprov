-- using default substitutions
SELECT *, mark_later(mapped_agg_later) FROM (
    SELECT 
        c_count,
        Count(*) AS custdist,
        agg_from_ptr(mapped_agg) as mapped_agg_later
    FROM   (SELECT c_custkey,
                Count(o_orderkey),
                agg_map_parallel(c_custkey, coalesce(o_orderkey, 0)) as mapped_agg
            FROM   customer
                LEFT OUTER JOIN orders
                                ON c_custkey = o_custkey
                                AND o_comment NOT LIKE '%special%requests%'
            GROUP  BY c_custkey) AS c_orders (c_custkey, c_count, mapped_agg)
GROUP  BY c_count) f
ORDER  BY custdist DESC, c_count DESC;