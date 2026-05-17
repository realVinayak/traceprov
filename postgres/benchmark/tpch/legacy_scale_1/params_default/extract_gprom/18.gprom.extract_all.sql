PROVENANCE OF (
    select c_name,
        c_custkey,
        o_orderkey,
        o_orderdate,
        o_totalprice,
        sum(l_quantity)
    from customer USE PROVENANCE (c_custkey),
        lineitem USE PROVENANCE (l_orderkey, l_linenumber),
        orders USE PROVENANCE (o_orderkey)
        JOIN (
            select l_orderkey as inner_l_orderkey
            from lineitem USE PROVENANCE (l_orderkey, l_linenumber)
            group by l_orderkey
            having sum(l_quantity) > 300
        ) a on a.inner_l_orderkey = o_orderkey
    where c_custkey = o_custkey
        and o_orderkey = l_orderkey
    group by c_name,
        c_custkey,
        o_orderkey,
        o_orderdate,
        o_totalprice
    order by o_totalprice desc,
        o_orderdate
    LIMIT 100
);