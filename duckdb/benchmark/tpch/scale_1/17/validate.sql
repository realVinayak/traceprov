-- using default substitutions
select sum(l_extendedprice) / 7.0 as avg_yearly
from lineitem,
    part
where p_partkey = l_partkey
    and (l_orderkey, l_linenumber, p_partkey) in (
        select column_1,
            column_2,
            column_3
        from traceprov_lineage_3
    )
    and l_quantity < (
        select 0.2 * avg(l_quantity)
        from lineitem
        where l_partkey = p_partkey
            and (l_orderkey, l_linenumber) in (
                select column_1_1,
                    column_2_1
                from traceprov_lineage_1
            )
    );