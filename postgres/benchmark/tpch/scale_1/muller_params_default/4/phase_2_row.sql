SELECT "orderby"."tuid" AS "tuid",
       "subquery3"."o_orderkey" AS "o_orderkey"
FROM (
              SELECT "group"."tuid" AS "tuid",
                     concat_agg("subquery2"."o_orderkey") AS "o_orderkey"
              FROM (
                            SELECT "join"."tuid" AS "tuid",
                                   "RTE0"."o_orderkey" AS "o_orderkey"
                            FROM orders_2_row AS "RTE0"(
                                          "tuid",
                                          "o_orderkey"
                                   ),
                                   LATERAL readjoin(
                                          2,
                                          "RTE0"."tuid"
                                   ) AS "join"("tuid")
                     ) AS "subquery2"(
                            "tuid",
                            "o_orderkey"
                     ),
                     LATERAL readaggregation(
                            3,
                            "subquery2"."tuid"
                     ) AS "group"("tuid")
              GROUP BY ("group"."tuid")
       ) AS "subquery3"(
              "tuid",
              "o_orderkey"
       ),
       LATERAL readorderby(5, "subquery3"."tuid") AS "orderby"("tuid");