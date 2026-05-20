SELECT prov_customer_c__custkey,
    prov_lineitem_l__orderkey,
    prov_lineitem_l__linenumber,
    prov_orders_o__orderkey,
    "prov_lineitem_1_l__orderkey",
    "prov_lineitem_1_l__linenumber"
FROM (
        PROVENANCE OF (
            select c_name,
                c_custkey,
                o_orderkey,
                o_orderdate,
                o_totalprice,
                sum(l_quantity)
            from customer,
                orders,
                lineitem
            where o_orderkey in (
                    select l_orderkey
                    from lineitem
                    group by l_orderkey
                    having sum(l_quantity) > 300
                )
                and c_custkey = o_custkey
                and o_orderkey = l_orderkey
            group by c_name,
                c_custkey,
                o_orderkey,
                o_orderdate,
                o_totalprice
            order by o_totalprice desc,
                o_orderdate
            LIMIT 100
        )
    );