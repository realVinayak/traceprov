-- using default substitutions
select
    supp_nation,
    cust_nation,
    l_year,
    sum(volume) as revenue
from
    (
        select
            n1.n_name as supp_nation,
            n2.n_name as cust_nation,
            extract(
                year
                from
                    l_shipdate
            ) as l_year,
            l_extendedprice * (1 - l_discount) as volume
        from
            supplier,
            lineitem,
            orders,
            customer,
            nation n1,
            nation n2
        where
            s_suppkey = l_suppkey
            and o_orderkey = l_orderkey
            and c_custkey = o_custkey
            and s_nationkey = n1.n_nationkey
            and c_nationkey = n2.n_nationkey
            and (
                supplier.rowid,
                lineitem.rowid,
                orders.rowid,
                customer.rowid,
                n1.rowid,
                n2.rowid
            ) in (
                select
                    column_1,
                    column_2,
                    column_3,
                    column_4,
                    column_5,
                    column_6
                from
                    LAYER_1_%OUT_ID%
            )
    ) as shipping
group by
    supp_nation,
    cust_nation,
    l_year
order by
    supp_nation,
    cust_nation,
    l_year;