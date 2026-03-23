-- using default substitutions
select sum(l_extendedprice * l_discount) as revenue
from lineitem
where lineitem.rowid in (
    select iid
    from LAYER_1_SD_%OUT_ID%
    where "table" = 0
)