PROVENANCE OF (
    select c_name,
        c_custkey,
        o_orderkey,
        o_orderdate,
        o_totalprice,
        sum(l_quantity)
    from customer USE PROVENANCE (c_custkey),
        orders USE PROVENANCE (o_orderkey),
        lineitem USE PROVENANCE (l_orderkey, l_linenumber)
    where o_orderkey in (
            select l_orderkey
            from lineitem USE PROVENANCE (l_orderkey, l_linenumber)
            group by l_orderkey
            having sum(l_quantity) > 300
        )
        and c_custkey = o_custkey
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