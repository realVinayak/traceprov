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
    and (ps_partkey, ps_suppkey, p_partkey) in (
        select
            column_1,
            column_2,
            column_3
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