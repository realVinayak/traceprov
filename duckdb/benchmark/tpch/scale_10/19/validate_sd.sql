-- using default substitutions
select sum(l_extendedprice * (1 - l_discount)) as revenue
from lineitem,
    part
where p_partkey = l_partkey
    and lineitem.rowid in (
        select iid
        from LAYER_1_SD_%OUT_ID%
        where "table" = 0
    )
    and part.rowid in (
        select iid
        from LAYER_1_SD_%OUT_ID%
        where "table" = 2
    );