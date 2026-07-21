-- using default substitutions
select sum(l_extendedprice) / 7.0 as avg_yearly
from lineitem,
    part
where p_partkey = l_partkey
    and (lineitem.rowid in (select iid from LAYER_1_SD_%OUT_ID% where "table" = 11))
    and (part.rowid in (select iid from LAYER_1_SD_%OUT_ID% where "table" = 12))
    and l_quantity < (
        select 0.2 * avg(l_quantity)
        from lineitem
        where l_partkey = p_partkey
    );