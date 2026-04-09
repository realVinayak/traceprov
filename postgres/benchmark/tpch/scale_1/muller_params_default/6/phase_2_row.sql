SELECT "group"."tuid" AS "tuid",
       concat_agg("subquery1"."l_orderkey") AS "l_orderkey"
FROM (
              SELECT "join"."tuid" AS "tuid",
                     "RTE0"."l_orderkey" AS "l_orderkey"
              FROM lineitem_2_row AS "RTE0"(
                            "tuid",
                            "l_orderkey"
                     ),
                     LATERAL readjoin(1, "RTE0"."tuid") AS "join"("tuid")
       ) AS "subquery1"(
              "tuid",
              "l_orderkey"
       ),
       LATERAL readaggregation(
              2,
              "subquery1"."tuid"
       ) AS "group"("tuid")
GROUP BY ("group"."tuid");