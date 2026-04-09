SELECT "orderby"."tuid" AS "tuid",
       "subquery7"."p_partkey" AS "p_partkey",
       "subquery7"."s_suppkey" AS "s_suppkey",
       "subquery7"."l_orderkey" AS "l_orderkey",
       "subquery7"."ps_partkey" AS "ps_partkey",
       "subquery7"."o_orderkey" AS "o_orderkey",
       "subquery7"."n_nationkey" AS "n_nationkey"
FROM (
              SELECT "group"."tuid" AS "tuid",
                     concat_agg("subquery6"."p_partkey") AS "p_partkey",
                     concat_agg("subquery6"."s_suppkey") AS "s_suppkey",
                     concat_agg("subquery6"."l_orderkey") AS "l_orderkey",
                     concat_agg("subquery6"."ps_partkey") AS "ps_partkey",
                     concat_agg("subquery6"."o_orderkey") AS "o_orderkey",
                     concat_agg("subquery6"."n_nationkey") AS "n_nationkey"
              FROM (
                            SELECT "join"."tuid" AS "tuid",
                                   "RTE0"."p_partkey" as "p_partkey",
                                   "RTE1"."s_suppkey" as "s_suppkey",
                                   "RTE2"."l_orderkey" as "l_orderkey",
                                   "RTE3"."ps_partkey" as "ps_partkey",
                                   "RTE4"."o_orderkey" as "o_orderkey",
                                   "RTE5"."n_nationkey" as "n_nationkey"
                            FROM part_2_row AS "RTE0"(
                                          "tuid",
                                          "p_partkey"
                                   ),
                                   supplier_2_row AS "RTE1"(
                                          "tuid",
                                          "s_suppkey"
                                   ),
                                   lineitem_2_row AS "RTE2"(
                                          "tuid",
                                          "l_orderkey"
                                   ),
                                   partsupp_2_row AS "RTE3"(
                                          "tuid",
                                          "ps_partkey"
                                   ),
                                   orders_2_row AS "RTE4"(
                                          "tuid",
                                          "o_orderkey"
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
                            "p_partkey",
                            "s_suppkey",
                            "l_orderkey",
                            "ps_partkey",
                            "o_orderkey",
                            "n_nationkey"
                     ),
                     LATERAL readaggregation(
                            2,
                            "subquery6"."tuid"
                     ) AS "group"("tuid")
              GROUP BY ("group"."tuid")
       ) AS "subquery7"(
              "tuid",
              "p_partkey",
              "s_suppkey",
              "l_orderkey",
              "ps_partkey",
              "o_orderkey",
              "n_nationkey"
       ),
       LATERAL readorderby(4, "subquery7"."tuid") AS "orderby"("tuid");