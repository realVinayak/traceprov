-- using default substitutions
select *,
    traceprov_log_entry_1 (2, mapped_agg)
FROM (
        select supp_nation,
            cust_nation,
            l_year,
            sum(volume) as revenue,
            traceprov_agg_key_parallel_offset_6 (
                1,
                (shipping.tp_s_suppkey::int),
                (shipping.tp_l_orderkey::int),
                (shipping.tp_o_orderkey::int),
                (shipping.tp_c_custkey::int),
                (shipping.tp_n_nationkey::int),
                (shipping.tp_n_nationkey_1::int)
            ) as mapped_agg
        from (
                select n1.n_name as supp_nation,
                    n2.n_name as cust_nation,
                    extract(
                        year
                        from l_shipdate
                    ) as l_year,
                    l_extendedprice * (1 - l_discount) as volume,
                    supplier.rowid AS tp_s_suppkey,
                    lineitem.rowid AS tp_l_orderkey,
                    orders.rowid AS tp_o_orderkey,
                    customer.rowid AS tp_c_custkey,
                    n1.rowid AS tp_n_nationkey,
                    n2.rowid AS tp_n_nationkey_1
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
                    and (
                        (
                            n1.n_name = 'FRANCE'
                            and n2.n_name = 'GERMANY'
                        )
                        or (
                            n1.n_name = 'GERMANY'
                            and n2.n_name = 'FRANCE'
                        )
                    )
                    and l_shipdate between date '1995-01-01' and date '1996-12-31'
            ) as shipping
        group by supp_nation,
            cust_nation,
            l_year
        order by supp_nation,
            cust_nation,
            l_year
    ) F