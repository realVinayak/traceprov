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
            and (
                p_partkey,
                s_suppkey,
                l_orderkey,
                l_linenumber,
                o_orderkey,
                c_custkey,
                n1.n_nationkey,
                n2.n_nationkey,
                r_regionkey
            ) in (
                select col_1,
                    col_2,
                    col_3,
                    col_4,
                    col_5,
                    col_6,
                    col_7,
                    col_8,
                    col_9
                from traceprov_relation_infer_1_mat
            )
    ) as all_nations
group by o_year
order by o_year;