-- using default substitutions
SELECT *, mark_later(mapped_agg) FROM (
    SELECT   c_custkey,
            c_name,
            SUM(l_extendedprice * (1 - l_discount)) AS revenue,
            c_acctbal,
            n_name,
            c_address,
            c_phone,
            c_comment,
            agg_map_parallel(
                c_custkey,
                o_orderkey,
                l_orderkey,
                l_linenumber,
                n_nationkey
            ) as mapped_agg
    FROM     customer,
            orders,
            lineitem,
            nation
    WHERE    c_custkey = o_custkey
    AND      l_orderkey = o_orderkey
    AND      o_orderdate >= DATE '1993-10-01'
    AND      o_orderdate <  DATE '1993-10-01' + interval '3' month
    AND      l_returnflag = 'R'
    AND      c_nationkey = n_nationkey
    GROUP BY c_custkey,
            c_name,
            c_acctbal,
            c_phone,
            n_name,
            c_address,
            c_comment
    ) f
ORDER BY revenue DESC
LIMIT 20;