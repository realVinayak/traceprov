-- using default substitutions
select
    o_year,
    sum(
        case
            when nation = 'BRAZIL' then volume
            else 0
        end
    ) / sum(volume) as mkt_share
from
    (
        select
            extract(
                year
                from
                    o_orderdate
            ) as o_year,
            l_extendedprice * (1 - l_discount) as volume,
            n2.n_name as nation
        from
            part,
            supplier,
            lineitem,
            orders,
            customer,
            nation n1,
            nation n2,
            region
        where
            p_partkey = l_partkey
            and s_suppkey = l_suppkey
            and l_orderkey = o_orderkey
            and o_custkey = c_custkey
            and c_nationkey = n1.n_nationkey
            and n1.n_regionkey = r_regionkey
            and s_nationkey = n2.n_nationkey
            and (
                part.rowid,
                supplier.rowid,
                lineitem.rowid,
                orders.rowid,
                customer.rowid,
                n1.rowid,
                n2.rowid,
                region.rowid
            ) in (
                select
                    column_0,
                    column_1,
                    column_2,
                    column_3,
                    column_4,
                    column_5,
                    column_6,
                    column_7
                from
                    LAYER_1
            )
    ) as all_nations
group by
    o_year
order by
    o_year;