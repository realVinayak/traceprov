-- using default substitutions
select o_year,
    sum(
        case
            when nation = 'BRAZIL' then volume
            else 0
        end
    ) / sum(volume) as mkt_share
from (
        select extract(
                year
                from o_orderdate
            ) as o_year,
            l_extendedprice * (1 - l_discount) as volume,
            n2.n_name as nation
        from part,
            supplier,
            lineitem,
            orders,
            customer,
            nation n1,
            nation n2,
            region
        where p_partkey = l_partkey
            and s_suppkey = l_suppkey
            and l_orderkey = o_orderkey
            and o_custkey = c_custkey
            and c_nationkey = n1.n_nationkey
            and n1.n_regionkey = r_regionkey
            and s_nationkey = n2.n_nationkey
            and part.rowid in (
                select iid
                from LAYER_1_SD_%OUT_ID%
                where "table" = 4
            )
            and supplier.rowid in (
                select iid
                from LAYER_1_SD_%OUT_ID%
                where "table" = 0
            )
            and lineitem.rowid in (
                select iid
                from LAYER_1_SD_%OUT_ID%
                where "table" = 3
            )
            and orders.rowid in (
                select iid
                from LAYER_1_SD_%OUT_ID%
                where "table" = 2
            )
            and customer.rowid in (
                select iid
                from LAYER_1_SD_%OUT_ID%
                where "table" = 1
            )
            and n1.rowid in (
                select iid
                from LAYER_1_SD_%OUT_ID%
                where "table" = 8
            )
            and n2.rowid in (
                select iid
                from LAYER_1_SD_%OUT_ID%
                where "table" = 13
            )
            and region.rowid in (
                select iid
                from LAYER_1_SD_%OUT_ID%
                where "table" = 9
            )
    ) as all_nations
group by o_year
order by o_year;