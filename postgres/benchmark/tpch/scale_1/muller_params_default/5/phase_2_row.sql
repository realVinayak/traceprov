SELECT "orderby"."tuid" AS "tuid",
       "subquery7"."c_custkey" AS "c_custkey",
       "subquery7"."o_orderkey" AS "o_orderkey",
       "subquery7"."l_orderkey" AS "l_orderkey",
       "subquery7"."s_suppkey" AS "s_suppkey",
       "subquery7"."n_nationkey" AS "n_nationkey",
       "subquery7"."r_regionkey" AS "r_regionkey"
FROM (
              SELECT "group"."tuid" AS "tuid",
                     concat_agg("subquery6"."c_custkey") AS "c_custkey",
                     concat_agg("subquery6"."o_orderkey") AS "o_orderkey",
                     concat_agg("subquery6"."l_orderkey") AS "l_orderkey",
                     concat_agg("subquery6"."s_suppkey") AS "s_suppkey",
                     concat_agg("subquery6"."n_nationkey") AS "n_nationkey",
                     concat_agg("subquery6"."r_regionkey") AS "r_regionkey"
              FROM (
                            SELECT "join"."tuid" AS "tuid",
                                   "RTE0"."c_custkey" as "c_custkey",
                                   "RTE1"."o_orderkey" as "o_orderkey",
                                   "RTE2"."l_orderkey" as "l_orderkey",
                                   "RTE3"."s_suppkey" as "s_suppkey",
                                   "RTE4"."n_nationkey" as "n_nationkey",
                                   "RTE5"."r_regionkey" as "r_regionkey"
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
                                   supplier_2_row AS "RTE3"(
                                          "tuid",
                                          "s_suppkey"
                                   ),
                                   nation_2_row AS "RTE4"(
                                          "tuid",
                                          "n_nationkey"
                                   ),
                                   region_2_row AS "RTE5"(
                                          "tuid",
                                          "r_regionkey"
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
                            "c_custkey",
                            "o_orderkey",
                            "l_orderkey",
                            "s_suppkey",
                            "n_nationkey",
                            "r_regionkey"
                     ),
                     LATERAL readaggregation(
                            2,
                            "subquery6"."tuid"
                     ) AS "group"("tuid")
              GROUP BY ("group"."tuid")
       ) AS "subquery7"(
              "tuid",
              "c_custkey",
              "o_orderkey",
              "l_orderkey",
              "s_suppkey",
              "n_nationkey",
              "r_regionkey"
       ),
       LATERAL readorderby(4, "subquery7"."tuid") AS "orderby"("tuid");