PROVENANCE OF (
    select sum(l_extendedprice) / 7.0 as avg_yearly
    from lineitem USE PROVENANCE (l_orderkey, l_linenumber),
        part USE PROVENANCE (p_partkey)
    where p_partkey = l_partkey
        and p_brand = 'Brand#23'
        and p_container = 'MED BOX'
        and l_quantity < (
            select 0.2 * avg(l_quantity)
            from lineitem USE PROVENANCE (l_orderkey, l_linenumber)
            where l_partkey = p_partkey
        )
);