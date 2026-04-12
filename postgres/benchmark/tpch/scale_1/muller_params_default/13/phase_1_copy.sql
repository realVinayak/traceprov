SELECT writeorderby(
              6,
              "subquery5"."tuid",
              row_number() OVER(
                     ORDER BY "subquery5"."custdist" DESC,
                            "subquery5"."c_count" DESC RANGE UNBOUNDED PRECEDING
              )
       ) AS "tuid",
       "subquery5"."c_count" AS "c_count",
       "subquery5"."custdist" AS "custdist"
FROM (
              SELECT writeaggregation(
                            4,
                            array_agg("subquery4"."tuid")
                     ) AS "tuid",
                     "subquery4"."c_count" AS "c_count",
                     count(*) AS "custdist"
              FROM (
                            SELECT writeaggregation(
                                          2,
                                          array_agg("subquery3"."tuid")
                                   ) AS "tuid",
                                   "subquery3"."c_custkey" AS "c_custkey",
                                   count("subquery3"."o_orderkey") AS "c_count"
                            FROM (
                                          SELECT writejoinleft(
                                                        1,
                                                        "RTE0"."tuid",
                                                        "RTE1"."tuid"
                                                 ) AS "tuid",
                                                 "RTE0"."c_custkey" AS "c_custkey",
                                                 "RTE1"."o_orderkey" AS "o_orderkey"
                                          FROM (
                                                        customer_1 AS "RTE0"(
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
                                                        LEFT OUTER JOIN orders_1 AS "RTE1"(
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
                                                        ) ON (
                                                               "RTE0"."c_custkey" = "RTE1"."o_custkey"
                                                               AND "RTE1"."o_comment" !~~ '%special%requests%'
                                                        )
                                                 )
                                          where "RTE0".tuid <= 1010080
                                   ) AS "subquery3"(
                                          "tuid",
                                          "c_custkey",
                                          "o_orderkey"
                                   )
                            GROUP BY "subquery3"."c_custkey"
                            HAVING True
                     ) AS "subquery4"(
                            "tuid",
                            "c_custkey",
                            "c_count"
                     )
              GROUP BY "subquery4"."c_count"
              HAVING True
       ) AS "subquery5"(
              "tuid",
              "c_count",
              "custdist"
       )
ORDER BY "subquery5"."custdist" DESC,
       "subquery5"."c_count" DESC
LIMIT NULL;