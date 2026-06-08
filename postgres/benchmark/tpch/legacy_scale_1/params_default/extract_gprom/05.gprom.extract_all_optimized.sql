PROVENANCE OF (
    select n_name,
        sum(l_extendedprice * (1 - l_discount)) as revenue
    from customer USE PROVENANCE (c_custkey),
        orders USE PROVENANCE (o_orderkey),
        lineitem USE PROVENANCE (l_linenumber),
        supplier USE PROVENANCE (s_suppkey),
        nation USE PROVENANCE (n_nationkey),
        region USE PROVENANCE (r_regionkey)
    where c_custkey = o_custkey
        and l_orderkey = o_orderkey
        and l_suppkey = s_suppkey
        and c_nationkey = s_nationkey
        and s_nationkey = n_nationkey
        and n_regionkey = r_regionkey
        and r_name = 'ASIA'
        and o_orderdate >= '1994-01-01'
        and o_orderdate < '1995-01-01'
    group by n_name
    order by revenue desc
);