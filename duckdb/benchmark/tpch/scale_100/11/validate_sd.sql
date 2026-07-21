-- using default substitutions
select ps_partkey,
    sum(ps_supplycost * ps_availqty) as value
from partsupp,
    supplier,
    nation
where ps_suppkey = s_suppkey
    and s_nationkey = n_nationkey
    and (partsupp.rowid in (select iid from LAYER_1_SD_%OUT_ID% where "table" = 0))
    and (supplier.rowid in (select iid from LAYER_1_SD_%OUT_ID% where "table" = 1))
    and (nation.rowid in (select iid from LAYER_1_SD_%OUT_ID% where "table" = 2))
group by ps_partkey
having sum(ps_supplycost * ps_availqty) > (
        select sum(ps_supplycost * ps_availqty) * 0.0000010000
        from partsupp,
            supplier,
            nation
        where ps_suppkey = s_suppkey
            and s_nationkey = n_nationkey
            and (partsupp.rowid in (select iid from LAYER_1_SD_%OUT_ID% where "table" = 9))
            and (supplier.rowid in (select iid from LAYER_1_SD_%OUT_ID% where "table" = 10))
            and (nation.rowid in (select iid from LAYER_1_SD_%OUT_ID% where "table" = 11))
    )
order by value desc;