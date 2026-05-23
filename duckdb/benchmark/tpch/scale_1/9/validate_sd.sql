-- using default substitutions
select nation,
    o_year,
    sum(amount) as sum_profit
from (
        select n_name as nation,
            extract(
                year
                from o_orderdate
            ) as o_year,
            l_extendedprice * (1 - l_discount) - ps_supplycost * l_quantity as amount
        from part,
            supplier,
            lineitem,
            partsupp,
            orders,
            nation
        where s_suppkey = l_suppkey
            and ps_suppkey = l_suppkey
            and ps_partkey = l_partkey
            and p_partkey = l_partkey
            and o_orderkey = l_orderkey
            and s_nationkey = n_nationkey
            and part.rowid in (
                select iid
                from LAYER_1_SD_%OUT_ID%
                where "table" = 2
            )
            and supplier.rowid in (
                select iid
                from LAYER_1_SD_%OUT_ID%
                where "table" = 7
            )
            and lineitem.rowid in (
                select iid
                from LAYER_1_SD_%OUT_ID%
                where "table" = 1
            )
            and partsupp.rowid in (
                select iid
                from LAYER_1_SD_%OUT_ID%
                where "table" = 6
            )
            and orders.rowid in (
                select iid
                from LAYER_1_SD_%OUT_ID%
                where "table" = 0
            )
            and nation.rowid in (
                select iid
                from LAYER_1_SD_%OUT_ID%
                where "table" = 8
            )
    ) as profit
group by nation,
    o_year
order by nation,
    o_year desc;