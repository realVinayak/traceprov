SELECT "group"."tuid" AS "tuid",
       concat_agg("subquery2"."l_orderkey") as "l_orderkey",
       concat_agg("subquery2"."p_partkey") as "p_partkey"
FROM (
              SELECT "join"."tuid" AS "tuid",
                     "RTE0"."l_orderkey" AS "l_orderkey",
                     "RTE1"."p_partkey" AS "p_partkey"
              FROM lineitem_2_row AS "RTE0"(
                            "tuid",
                            "l_orderkey"
                     ),
                     part_2_row AS "RTE1"(
                            "tuid",
                            "p_partkey"
                     ),
                     LATERAL readjoin(
                            1,
                            "RTE0"."tuid",
                            "RTE1"."tuid"
                     ) AS "join"("tuid")
       ) AS "subquery2"(
              "tuid",
              "l_orderkey",
              "p_partkey"
       ),
       LATERAL readaggregation(
              2,
              "subquery2"."tuid"
       ) AS "group"("tuid")
GROUP BY ("group"."tuid");