-- using default substitutions
select p_brand,
    p_type,
    p_size,
    count(distinct ps_suppkey) as supplier_cnt
from partsupp,
    part
where p_partkey = ps_partkey
    and (partsupp.rowid) in (
        select iid
        from LAYER_1_SD_%OUT_ID%
        where "table" = 0
    )
    and (part.rowid) in (
        select iid
        from LAYER_1_SD_%OUT_ID%
        where "table" = 1
    )
group by p_brand,
    p_type,
    p_size
order by supplier_cnt desc,
    p_brand,
    p_type,
    p_size;