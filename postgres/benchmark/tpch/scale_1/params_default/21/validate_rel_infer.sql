select s_name,
    count(*) as numwait
from supplier,
    lineitem l1,
    orders,
    nation
where exists (
        select *
        from lineitem l2
        where l2.l_orderkey = l1.l_orderkey
            and (l2.l_orderkey, l2.l_linenumber) in (
                select col_8,
                    col_9
                from traceprov_relation_infer_1_mat
            )
    )
    and not exists (
        select *
        from lineitem l3
        where l3.l_orderkey = l1.l_orderkey
            and l3.l_suppkey <> l1.l_suppkey
            and l3.l_receiptdate > l3.l_commitdate
    )
    and (
        s_suppkey,
        l_orderkey,
        l_linenumber,
        o_orderkey,
        n_nationkey
    ) in (
        select col_1,
            col_2,
            col_3,
            col_4,
            col_5
        from traceprov_relation_infer_2_mat
    )
group by s_name
order by numwait desc,
    s_name
LIMIT 100;