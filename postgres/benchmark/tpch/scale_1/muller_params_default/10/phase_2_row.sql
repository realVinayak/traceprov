SELECT "orderby"."tuid" AS "tuid",
       "subquery5"."c_custkey" AS "c_custkey",
       "subquery5"."c_custkey" AS "c_custkey",
       "subquery5"."l_orderkey" AS "l_orderkey",
       "subquery5"."n_nationkey" AS "n_nationkey"
FROM (
              SELECT "group"."tuid" AS "tuid",
                     concat_agg("subquery4"."c_custkey") AS "c_custkey",
                     concat_agg("subquery4"."o_orderkey") AS "o_orderkey",
                     concat_agg("subquery4"."l_orderkey") AS "l_orderkey",
                     concat_agg("subquery4"."n_nationkey") AS "n_nationkey"
              FROM (
                            SELECT "join"."tuid" AS "tuid",
                                   "RTE0"."c_custkey" AS "c_custkey",
                                   "RTE1"."o_orderkey" AS "o_orderkey",
                                   "RTE2"."l_orderkey" as "l_orderkey",
                                   "RTE3"."n_nationkey" as "n_nationkey"
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
                                   nation_2_row AS "RTE3"(
                                          "tuid",
                                          "n_nationkey"
                                   ),
                                   LATERAL readjoin(
                                          1,
                                          "RTE0"."tuid",
                                          "RTE1"."tuid",
                                          "RTE2"."tuid",
                                          "RTE3"."tuid"
                                   ) AS "join"("tuid")
                     ) AS "subquery4"(
                            "tuid",
                            "c_custkey",
                            "o_orderkey",
                            "l_orderkey",
                            "n_nationkey"
                     ),
                     LATERAL readaggregation(
                            2,
                            "subquery4"."tuid"
                     ) AS "group"("tuid")
              GROUP BY ("group"."tuid")
       ) AS "subquery5"(
              "tuid",
              "c_custkey",
              "o_orderkey",
              "l_orderkey",
              "n_nationkey"
       ),
       LATERAL readorderby(4, "subquery5"."tuid") AS "orderby"("tuid");