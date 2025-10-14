PROVENANCE OF (
    select
        sum(l_extendedprice) / 7.0 as avg_yearly
    from
        part USE PROVENANCE (p_partkey),
        lineitem USE PROVENANCE (l_orderkey, l_linenumber) JOIN (
            SELECT 
                0.2 * avg(l_quantity) as avgdvalue,
                l_partkey as internal_lpartkey
            FROM lineitem USE PROVENANCE (l_orderkey, l_linenumber)
            group BY l_partkey
        ) as f
        ON l_quantity < f.avgdvalue
    where
        p_partkey = l_partkey
        and p_brand = 'Brand#23'
        and p_container = 'MED BOX'
        and f.internal_lpartkey = p_partkey
);
