-- SELECT prov_partsupp_ps__partkey,
--     prov_partsupp_ps__suppkey,
--     prov_supplier_s__suppkey,
--     prov_nation_n__nationkey,
--     prov_partsupp_1_ps__partkey,
--     prov_partsupp_1_ps__suppkey,
--     prov_supplier_1_s__suppkey,
--     prov_nation_1_n__nationkey
-- FROM (
--         PROVENANCE OF (
--             SELECT ps_partkey,
--                 value
--             FROM (
--                     select ps_partkey,
--                         sum(ps_supplycost * ps_availqty) as value
--                     from partsupp USE PROVENANCE (ps_partkey, ps_suppkey),
--                         supplier USE PROVENANCE (s_suppkey),
--                         nation USE PROVENANCE (n_nationkey)
--                     where ps_suppkey = s_suppkey
--                         and s_nationkey = n_nationkey
--                         and n_name = 'GERMANY'
--                     group by ps_partkey
--                 ) a
--                 JOIN (
--                     SELECT sum(ps_supplycost * ps_availqty) * 0.0001000000 as computed_value
--                     FROM partsupp USE PROVENANCE (ps_partkey, ps_suppkey),
--                         supplier USE PROVENANCE (s_suppkey),
--                         nation USE PROVENANCE (n_nationkey)
--                     WHERE ps_suppkey = s_suppkey
--                         and s_nationkey = n_nationkey
--                         and n_name = 'GERMANY'
--                 ) b on a.value > b.computed_value
--             order by value desc
--         )
--     );
SELECT prov_partsupp_ps__partkey,
    prov_partsupp_ps__suppkey,
    prov_supplier_s__suppkey,
    prov_nation_n__nationkey,
    "prov_partsupp_1_ps__partkey",
    "prov_partsupp_1_ps__suppkey",
    "prov_supplier_1_s__suppkey",
    "prov_nation_1_n__nationkey"
FROM (
        PROVENANCE OF (
            select *
            from (
                    select ps_partkey,
                        sum(ps_supplycost * ps_availqty) as value
                    from partsupp USE PROVENANCE (ps_partkey, ps_suppkey),
                        supplier USE PROVENANCE (s_suppkey),
                        nation USE PROVENANCE (n_nationkey)
                    where ps_suppkey = s_suppkey
                        and s_nationkey = n_nationkey
                        and n_name = 'GERMANY'
                    group by ps_partkey
                )
            where value > (
                    select sum(ps_supplycost * ps_availqty) * 0.0001000000
                    from partsupp USE PROVENANCE (ps_partkey, ps_suppkey),
                        supplier USE PROVENANCE (s_suppkey),
                        nation USE PROVENANCE (n_nationkey)
                    where ps_suppkey = s_suppkey
                        and s_nationkey = n_nationkey
                        and n_name = 'GERMANY'
                )
            order by value desc
        )
    );