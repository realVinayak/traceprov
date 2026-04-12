SELECT "orderby"."tuid" AS "tuid",
       "subquery8"."ps_partkey" AS "ps_partkey",
       "subquery8"."s_suppkey" AS "s_suppkey",
       "subquery8"."n_nationkey" AS "n_nationkey"
FROM (
              SELECT "group"."tuid" AS "tuid",
                     concat_agg("subquery3"."ps_partkey") AS "ps_partkey",
                     concat_agg("subquery3"."s_suppkey") AS "s_suppkey",
                     concat_agg("subquery3"."n_nationkey") AS "n_nationkey"
              FROM (
                            SELECT "join"."tuid" AS "tuid",
                                   "RTE0"."ps_partkey" AS "ps_partkey",
                                   "RTE1"."s_suppkey" AS "s_suppkey",
                                   "RTE2"."n_nationkey" AS "n_nationkey"
                            FROM partsupp_2_row AS "RTE0"(
                                          "tuid",
                                          "ps_partkey"
                                   ),
                                   supplier_2_row AS "RTE1"(
                                          "tuid",
                                          "s_suppkey"
                                   ),
                                   nation_2_row AS "RTE2"(
                                          "tuid",
                                          "n_nationkey"
                                   ),
                                   LATERAL readjoin(
                                          1,
                                          "RTE0"."tuid",
                                          "RTE1"."tuid",
                                          "RTE2"."tuid"
                                   ) AS "join"("tuid")
                     ) AS "subquery3"(
                            "tuid",
                            "ps_partkey",
                            "s_suppkey",
                            "n_nationkey"
                     ),
                     LATERAL readaggregation(
                            5,
                            "subquery3"."tuid"
                     ) AS "group"("tuid")
              GROUP BY ("group"."tuid")
       ) AS "subquery8"(
              "tuid",
              "ps_partkey",
              "s_suppkey",
              "n_nationkey"
       ),
       LATERAL readorderby(7, "subquery8"."tuid") AS "orderby"("tuid");