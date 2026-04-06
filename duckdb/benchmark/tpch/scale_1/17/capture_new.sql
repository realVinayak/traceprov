-- using default substitutions
select
    sum(l_extendedprice) / 7.0 as avg_yearly,
    traceprov_log_entry_1 (
        4,
        traceprov_agg_key_parallel_offset_2 (3, lineitem.rowid, part.rowid)
    )
from
    lineitem,
    part
where
    p_partkey = l_partkey
    and p_brand = 'Brand#23'
    and p_container = 'MED BOX'
    and l_quantity < (
        select
            avged
        from
            (
                select
                    0.2 * avg(l_quantity) as avged,
                    traceprov_log_entry_volatile_2 (
                        2,
                        part.rowid,
                        traceprov_agg_key_parallel_offset_1 (1, lineitem.rowid)
                    )
                from
                    lineitem
                where
                    l_partkey = p_partkey
            ) f
    );