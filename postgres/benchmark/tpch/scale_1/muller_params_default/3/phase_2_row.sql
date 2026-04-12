SELECT "orderby"."tuid" AS "tuid",
       "subquery4"."c_custkey" as "c_custkey",
       "subquery4"."c_custkey" as "o_orderkey",
       "subquery4"."l_orderkey" AS "l_orderkey"
FROM (
              SELECT "group"."tuid" AS "tuid",
                     concat_agg("subquery3"."c_custkey") as "c_custkey",
                     concat_agg("subquery3"."o_orderkey") as "o_orderkey",
                     concat_agg("subquery3"."l_orderkey") AS "l_orderkey"
              FROM (
                            SELECT "join"."tuid" AS "tuid",
                                   "RTE0"."c_custkey" as "c_custkey",
                                   "RTE1"."o_orderkey" as "o_orderkey",
                                   "RTE2"."l_orderkey" as "l_orderkey"
                            FROM customer_2_row AS "RTE0"(
                                          "tuid",
                                          "c_custkey"
                                   ),
                                   orders_2_row AS "RTE1"(
                                          "tuid",
                                          "o_orderkey"
                                   ),
                                   lineitem_2_row AS "RTE2"(
                                          "tuid",
                                          "l_orderkey"
                                   ),
                                   LATERAL readjoin(
                                          1,
                                          "RTE0"."tuid",
                                          "RTE1"."tuid",
                                          "RTE2"."tuid"
                                   ) AS "join"("tuid")
                     ) AS "subquery3"(
                            "tuid",
                            "c_custkey",
                            "o_orderkey",
                            "l_orderkey"
                     ),
                     LATERAL readaggregation(
                            2,
                            "subquery3"."tuid"
                     ) AS "group"("tuid")
              GROUP BY ("group"."tuid")
       ) AS "subquery4"(
              "tuid",
              "c_custkey",
              "o_orderkey",
              "l_orderkey"
       ),
       LATERAL readorderby(4, "subquery4"."tuid") AS "orderby"("tuid");