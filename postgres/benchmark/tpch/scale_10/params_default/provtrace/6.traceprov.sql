-- using default substitutions
SELECT *, mark_later(mapped_agg) FROM (
    SELECT SUM(l_extendedprice * l_discount) AS revenue,
    traceprov_agg_key_parallel(1, l_orderkey, l_linenumber) as mapped_agg
    FROM   lineitem
    WHERE  l_shipdate >= DATE '1994-01-01'
        AND l_shipdate < DATE '1994-01-01' + interval '1' year
        AND l_discount BETWEEN .06 - 0.01 AND .06 + 0.01
        AND l_quantity < 24 
) f; 