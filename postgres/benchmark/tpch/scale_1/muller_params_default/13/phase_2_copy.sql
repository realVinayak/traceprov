SELECT "orderby"."tuid" AS "tuid",
       "subquery5"."c_count" AS "c_count",
       "subquery5"."custdist" AS "custdist"
FROM (
              SELECT "group"."tuid" AS "tuid",
                     concat_agg("subquery4"."c_count") AS "c_count",
                     ARRAY []::int4 [] AS "custdist"
              FROM (
                            SELECT "group"."tuid" AS "tuid",
                                   concat_agg("subquery3"."c_custkey") AS "c_custkey",
                                   concat_agg("subquery3"."o_orderkey") AS "c_count"
                            FROM (
                                          SELECT "join"."tuid" AS "tuid",
                                                 "RTE0"."c_custkey" AS "c_custkey",
                                                 "RTE1"."o_orderkey" AS "o_orderkey"
                                          FROM (
                                                        (
                                                               customer_2 AS "RTE0"(
                                                                      "tuid",
                                                                      "c_custkey",
                                                                      "c_name",
                                                                      "c_address",
                                                                      "c_nationkey",
                                                                      "c_phone",
                                                                      "c_acctbal",
                                                                      "c_mktsegment",
                                                                      "c_comment"
                                                               )
                                                               CROSS JOIN LATERAL readjoinleft(
                                                                      1,
                                                                      "RTE0"."tuid"
                                                               ) AS "join"(
                                                                      "tuid",
                                                                      "right"
                                                               )
                                                        )
                                                        LEFT OUTER JOIN orders_2 AS "RTE1"(
                                                               "tuid",
                                                               "o_orderkey",
                                                               "o_custkey",
                                                               "o_orderstatus",
                                                               "o_totalprice",
                                                               "o_orderdate",
                                                               "o_orderpriority",
                                                               "o_clerk",
                                                               "o_shippriority",
                                                               "o_comment"
                                                        ) ON "join"."right" = "RTE1"."tuid"
                                                 )
                                   ) AS "subquery3"(
                                          "tuid",
                                          "c_custkey",
                                          "o_orderkey"
                                   ),
                                   LATERAL readaggregation(
                                          2,
                                          "subquery3"."tuid"
                                   ) AS "group"("tuid")
                            GROUP BY "group"."tuid"
                     ) AS "subquery4"(
                            "tuid",
                            "c_custkey",
                            "c_count"
                     ),
                     LATERAL readaggregation(
                            4,
                            "subquery4"."tuid"
                     ) AS "group"("tuid")
              GROUP BY "group"."tuid"
       ) AS "subquery5"(
              "tuid",
              "c_count",
              "custdist"
       ),
       LATERAL readorderby(
              6,
              "subquery5"."tuid"
       ) AS "orderby"("tuid");