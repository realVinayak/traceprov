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
        where (
                s_suppkey,
                l_orderkey,
                l_linenumber,
                o_orderkey,
                c_custkey,
                n1.n_nationkey,
                n2.n_nationkey
            ) in (
                select col_1,
                    col_2,
                    col_3,
                    col_4,
                    col_5,
                    col_6,
                    col_7
                from traceprov_relation_infer_1_mat
            )
    ) as shipping
group by supp_nation,
    cust_nation,
    l_year
order by supp_nation,
    cust_nation,
    l_year;