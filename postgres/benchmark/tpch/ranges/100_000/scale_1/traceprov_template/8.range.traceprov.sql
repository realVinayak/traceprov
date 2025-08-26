-- using default substitutions
SELECT *, mark_later(mapped_agg) FROM (
    SELECT o_year,
        SUM(CASE
                WHEN nation = 'BRAZIL' THEN volume
                ELSE 0
            END) / SUM(volume) AS mkt_share,
        agg_map_parallel(%0%) as mapped_agg
    FROM   (SELECT Extract(year FROM o_orderdate)       AS o_year,
                l_extendedprice * ( 1 - l_discount ) AS volume,
                n2.n_name                            AS nation,
                p_partkey,
                s_suppkey,
                l_orderkey,
                l_linenumber,
                o_orderkey,
                c_custkey,
                n1.n_nationkey as n1_nationkey,
                n2.n_nationkey as n2_nationkey,
                r_regionkey
            FROM   part,
                supplier,
                lineitem,
                orders,
                customer,
                nation n1,
                nation n2,
                region
            WHERE  p_partkey = l_partkey
                AND s_suppkey = l_suppkey
                AND l_orderkey = o_orderkey
                AND o_custkey = c_custkey
                AND c_nationkey = n1.n_nationkey
                AND n1.n_regionkey = r_regionkey
                AND r_name = 'AMERICA'
                AND s_nationkey = n2.n_nationkey
                AND o_orderdate BETWEEN DATE '1995-01-01' AND DATE '1996-12-31'
                AND p_type = 'ECONOMY ANODIZED STEEL') AS all_nations
    GROUP  BY o_year
) f
ORDER  BY o_year; 