SELECT "orderby"."tuid" AS "tuid",
       "subquery7"."s_suppkey" as "s_suppkey",
       "subquery7"."l_orderkey" as "l_orderkey",
       "subquery7"."o_orderkey" as "o_orderkey",
       "subquery7"."c_custkey" as "c_custkey",
       "subquery7"."n_nationkey" as "n_nationkey",
       "subquery7"."n_nationkey_1" as "n_nationkey_1"
FROM (
              SELECT "group"."tuid" AS "tuid",
                     concat_agg("subquery6"."s_suppkey") AS "s_suppkey",
                     concat_agg("subquery6"."l_orderkey") AS "l_orderkey",
                     concat_agg("subquery6"."o_orderkey") AS "o_orderkey",
                     concat_agg("subquery6"."c_custkey") AS "c_custkey",
                     concat_agg("subquery6"."n_nationkey") AS "n_nationkey",
                     concat_agg("subquery6"."n_nationkey_1") AS "n_nationkey_1"
              FROM (
                            SELECT "join"."tuid" AS "tuid",
                                   "RTE0"."s_suppkey" as "s_suppkey",
                                   "RTE1"."l_orderkey" as "l_orderkey",
                                   "RTE2"."o_orderkey" as "o_orderkey",
                                   "RTE3"."c_custkey" as "c_custkey",
                                   "RTE4"."n_nationkey" as "n_nationkey",
                                   "RTE5"."n_nationkey" as "n_nationkey_1"
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
                                   customer_2_row AS "RTE3"(
                                          "tuid",
                                          "c_custkey"
                                   ),
                                   nation_2_row AS "RTE4"(
                                          "tuid",
                                          "n_nationkey"
                                   ),
                                   nation_2_row AS "RTE5"(
                                          "tuid",
                                          "n_nationkey"
                                   ),
                                   LATERAL readjoin(
                                          1,
                                          "RTE0"."tuid",
                                          "RTE1"."tuid",
                                          "RTE2"."tuid",
                                          "RTE3"."tuid",
                                          "RTE4"."tuid",
                                          "RTE5"."tuid"
                                   ) AS "join"("tuid")
                     ) AS "subquery6"(
                            "tuid",
                            "s_suppkey",
                            "l_orderkey",
                            "o_orderkey",
                            "c_custkey",
                            "n_nationkey",
                            "n_nationkey_1"
                     ),
                     LATERAL readaggregation(
                            2,
                            "subquery6"."tuid"
                     ) AS "group"("tuid")
              GROUP BY ("group"."tuid")
       ) AS "subquery7"(
              "tuid",
              "s_suppkey",
              "l_orderkey",
              "o_orderkey",
              "c_custkey",
              "n_nationkey",
              "n_nationkey_1"
       ),
       LATERAL readorderby(4, "subquery7"."tuid") AS "orderby"("tuid");