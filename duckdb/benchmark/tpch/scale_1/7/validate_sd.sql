-- using default substitutions
select supp_nation,
    cust_nation,
    l_year,
    sum(volume) as revenue
from (
        select n1.n_name as supp_nation,
            n2.n_name as cust_nation,
            extract(
                year
                from l_shipdate
            ) as l_year,
            l_extendedprice * (1 - l_discount) as volume
        from supplier,
            lineitem,
            orders,
            customer,
            nation n1,
            nation n2
        where s_suppkey = l_suppkey
            and o_orderkey = l_orderkey
            and c_custkey = o_custkey
            and s_nationkey = n1.n_nationkey
            and c_nationkey = n2.n_nationkey
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
            and orders.rowid in (
                select iid
                from LAYER_1_SD_%OUT_ID%
                where "table" = 0
            )
            and customer.rowid in (
                select iid
                from LAYER_1_SD_%OUT_ID%
                where "table" = 3
            )
            and n1.rowid in (
                select iid
                from LAYER_1_SD_%OUT_ID%
                where "table" = 8
            )
            and n2.rowid in (
                select iid
                from LAYER_1_SD_%OUT_ID%
                where "table" = 4
            )
    ) as shipping
group by supp_nation,
    cust_nation,
    l_year
order by supp_nation,
    cust_nation,
    l_year;