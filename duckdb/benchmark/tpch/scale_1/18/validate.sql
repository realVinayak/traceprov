-- using default substitutions
select c_name,
    c_custkey,
    o_orderkey,
    o_orderdate,
    o_totalprice,
    sum(l_quantity)
from customer,
    orders,
    lineitem
where c_custkey = o_custkey
    and o_orderkey = l_orderkey
    and o_orderkey in (
        select l_orderkey
        from lineitem
        where (l_orderkey, l_linenumber) in (
                select (column_1, column_2)
                from traceprov_lineage_1
            )
        group by l_orderkey
        having sum(l_quantity) > 300
    )
    and (c_custkey, o_orderkey, l_orderkey, l_linenumber) in (
        select (
                column_1,
                column_2,
                column_3,
                column_4
            )
        FROM traceprov_lineage_3
    )
group by c_name,
    c_custkey,
    o_orderkey,
    o_orderdate,
    o_totalprice
order by o_totalprice desc,
    o_orderdate
LIMIT 100;