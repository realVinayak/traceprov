-- using default substitutions
select
    nation,
    o_year,
    sum(amount) as sum_profit
from
    (
        select
            n_name as nation,
            extract(
                year
                from
                    o_orderdate
            ) as o_year,
            l_extendedprice * (1 - l_discount) - ps_supplycost * l_quantity as amount
        from
            part,
            supplier,
            lineitem,
            partsupp,
            orders,
            nation
        where
            s_suppkey = l_suppkey
            and ps_suppkey = l_suppkey
            and ps_partkey = l_partkey
            and p_partkey = l_partkey
            and o_orderkey = l_orderkey
            and s_nationkey = n_nationkey
            and (
                part.rowid,
                supplier.rowid,
                lineitem.rowid,
                partsupp.rowid,
                orders.rowid,
                nation.rowid
            ) in (
                select
                    opid_15_part,
                    opid_22_supplier,
                    opid_14_lineitem,
                    opid_20_partsupp,
                    opid_17_orders,
                    opid_23_nation
                from
                    LAYER_1
            )
    ) as profit
group by
    nation,
    o_year
order by
    nation,
    o_year desc;
