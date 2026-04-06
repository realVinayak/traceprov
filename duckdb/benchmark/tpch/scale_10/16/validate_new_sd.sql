-- using default substitutions
select
    p_brand,
    p_type,
    p_size,
    count(distinct ps_suppkey) as supplier_cnt
from
    partsupp,
    part
where
    p_partkey = ps_partkey
    and (partsupp.rowid, part.rowid) in (
        select
            opid_10_partsupp,
            opid_13_part
        from
            LAYER_1
    )
group by
    p_brand,
    p_type,
    p_size
order by
    supplier_cnt desc,
    p_brand,
    p_type,
    p_size;