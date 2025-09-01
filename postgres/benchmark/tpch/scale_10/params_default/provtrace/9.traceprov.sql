-- using default substitutions
SELECT *, mark_later(mapped_agg) FROM 
(
    SELECT nation,
        o_year,
        SUM(amount) AS sum_profit,
        traceprov_agg_key_parallel(
                1,
                p_partkey,
                s_suppkey,
                l_orderkey,
                l_linenumber,
                ps_partkey,
                ps_suppkey,
                o_orderkey,
                n_nationkey
        ) as mapped_agg
    FROM   (SELECT n_name
                AS
                        nation,
                Extract(year FROM o_orderdate)
                AS
                        o_year,
                l_extendedprice * ( 1 - l_discount ) - ps_supplycost * l_quantity
                AS
                        amount,
                p_partkey,
                s_suppkey,
                l_orderkey,
                l_linenumber,
                ps_partkey,
                ps_suppkey,
                o_orderkey,
                n_nationkey
            FROM   part,
                supplier,
                lineitem,
                partsupp,
                orders,
                nation
            WHERE  s_suppkey = l_suppkey
                AND ps_suppkey = l_suppkey
                AND ps_partkey = l_partkey
                AND p_partkey = l_partkey
                AND o_orderkey = l_orderkey
                AND s_nationkey = n_nationkey
                AND p_name LIKE '%green%') AS profit
    GROUP  BY nation,
            o_year
) f
ORDER  BY nation,
          o_year DESC;