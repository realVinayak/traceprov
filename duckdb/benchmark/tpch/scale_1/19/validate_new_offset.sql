-- using default substitutions
select sum(l_extendedprice * (1 - l_discount)) as revenue
from lineitem,
    part
where p_partkey = l_partkey
    and (lineitem.rowid, part.rowid) in (
        select (column_1, column_2)
        FROM LAYER_1_%OUT_ID%
    );