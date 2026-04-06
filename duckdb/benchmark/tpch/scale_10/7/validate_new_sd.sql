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
                    opid_19_supplier,
                    opid_17_lineitem,
                    opid_14_orders,
                    opid_23_customer,
                    opid_21_nation,
                    opid_25_nation
                from
                    LAYER_1
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