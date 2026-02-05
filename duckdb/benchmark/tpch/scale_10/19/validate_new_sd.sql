-- using default substitutions
select
    sum(l_extendedprice * (1 - l_discount)) as revenue
from
    lineitem,
    part
where
    p_partkey = l_partkey
    and (lineitem.rowid, part.rowid) in (
        select
            opid_8_lineitem,
            opid_10_part
        FROM
            LAYER_1
    );
