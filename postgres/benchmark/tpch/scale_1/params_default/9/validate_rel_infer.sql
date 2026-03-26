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
            and (
                (
                    p_partkey,
                    s_suppkey,
                    l_orderkey,
                    l_linenumber,
                    ps_partkey,
                    ps_suppkey,
                    o_orderkey,
                    n_nationkey
                ) in (
                    select col_1,
                        col_2,
                        col_3,
                        col_4,
                        col_5,
                        col_6,
                        col_7,
                        col_8
                    from traceprov_relation_infer_1_mat
                )
            )
    ) as profit
group by nation,
    o_year
order by nation,
    o_year desc;