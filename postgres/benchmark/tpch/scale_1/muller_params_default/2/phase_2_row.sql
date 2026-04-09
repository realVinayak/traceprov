SELECT "orderby"."tuid" AS "tuid",
       "subquery9"."p_partkey",
       "subquery9"."s_suppkey",
       "subquery9"."ps_partkey",
       "subquery9"."n_nationkey",
       "subquery9"."r_regionkey"
FROM (
              SELECT "join"."tuid" AS "tuid",
                     "RTE0".p_partkey as p_partkey,
                     "RTE1".s_suppkey as s_suppkey,
                     "RTE2".ps_partkey as ps_partkey,
                     "RTE3".n_nationkey as n_nationkey,
                     "RTE4".r_regionkey as r_regionkey
              FROM part_2_row AS "RTE0"("tuid", "p_partkey"),
                     supplier_2_row AS "RTE1"(
                            "tuid",
                            "s_suppkey"
                     ),
                     partsupp_2_row AS "RTE2"(
                            "tuid",
                            "ps_partkey"
                     ),
                     nation_2_row AS "RTE3"(
                            "tuid",
                            "n_nationkey"
                     ),
                     region_2_row AS "RTE4"(
                            "tuid",
                            "r_regionkey"
                     ),
                     LATERAL readjoin(
                            2,
                            "RTE0"."tuid",
                            "RTE1"."tuid",
                            "RTE2"."tuid",
                            "RTE3"."tuid",
                            "RTE4"."tuid"
                     ) AS "join"("tuid")
       ) AS "subquery9"(
              "tuid",
              "p_partkey",
              "s_suppkey",
              "ps_partkey",
              "n_nationkey",
              "r_regionkey"
       ),
       LATERAL readorderby(3, "subquery9"."tuid") AS "orderby"("tuid");