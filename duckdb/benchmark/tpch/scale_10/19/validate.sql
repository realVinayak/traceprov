-- using default substitutions
select
    sum(l_extendedprice * (1 - l_discount)) as revenue
from
    lineitem,
    part
where
    p_partkey = l_partkey
    and (l_orderkey, l_linenumber, p_partkey) in (
        select
            column_1,
            column_2,
            column_3
        FROM
            LAYER_1
    );