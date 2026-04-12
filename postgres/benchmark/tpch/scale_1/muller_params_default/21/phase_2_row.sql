SELECT "orderby"."tuid" AS "tuid",
       "subquery7"."s_suppkey" AS "s_name",
       "subquery7"."l_orderkey" AS "l_orderkey",
       "subquery7"."o_orderkey" AS "o_orderkey",
       "subquery7"."n_nationkey" AS "n_nationkey"
FROM (
              SELECT "group"."tuid" AS "tuid",
                     concat_agg("subquery6"."s_suppkey") AS "s_suppkey",
                     concat_agg("subquery6"."l_orderkey") AS "l_orderkey",
                     concat_agg("subquery6"."o_orderkey") AS "o_orderkey",
                     concat_agg("subquery6"."n_nationkey") AS "n_nationkey"
              FROM (
                            SELECT "join"."tuid" AS "tuid",
                                   "RTE0"."s_suppkey" AS "s_suppkey",
                                   "RTE1"."l_orderkey" AS "l_orderkey",
                                   "RTE2"."o_orderkey" AS "o_orderkey",
                                   "RTE3"."n_nationkey" AS "n_nationkey"
                            FROM supplier_2_row AS "RTE0"(
                                          "tuid",
                                          "s_suppkey"
                                   ),
                                   lineitem_2_row AS "RTE1"(
                                          "tuid",
                                          "l_orderkey"
                                   ),
                                   orders_2_row AS "RTE2"(
                                          "tuid",
                                          "o_orderkey"
                                   ),
                                   nation_2_row AS "RTE3"(
                                          "tuid",
                                          "n_nationkey"
                                   ),
                                   LATERAL readjoin(
                                          3,
                                          "RTE0"."tuid",
                                          "RTE1"."tuid",
                                          "RTE2"."tuid",
                                          "RTE3"."tuid"
                                   ) AS "join"("tuid")
                     ) AS "subquery6"(
                            "tuid",
                            "s_suppkey",
                            "l_orderkey",
                            "o_orderkey",
                            "n_nationkey"
                     ),
                     LATERAL readaggregation(
                            4,
                            "subquery6"."tuid"
                     ) AS "group"("tuid")
              GROUP BY ("group"."tuid")
       ) AS "subquery7"(
              "tuid",
              "s_suppkey",
              "l_orderkey",
              "o_orderkey",
              "n_nationkey"
       ),
       LATERAL readorderby(6, "subquery7"."tuid") AS "orderby"("tuid");