SELECT writeorderby(7, 
                    "subquery8"."tuid", 
                    row_number() OVER(ORDER BY ("subquery8"."value") DESC RANGE UNBOUNDED PRECEDING)) AS "tuid", 
       "subquery8"."ps_partkey" AS "ps_partkey", 
       "subquery8"."value" AS "value"
FROM (SELECT writeaggregation(5, 
                              array_agg("subquery3"."tuid")) AS "tuid", 
             "subquery3"."ps_partkey" AS "ps_partkey", 
             sum(("subquery3"."ps_supplycost") *
                 ("subquery3"."ps_availqty")) AS "value"
      FROM (SELECT writejoin(1, 
                             "RTE0"."tuid", 
                             "RTE1"."tuid", 
                             "RTE2"."tuid") AS "tuid", 
                   "RTE0"."ps_partkey" AS "ps_partkey", 
                   "RTE0"."ps_supplycost" AS "ps_supplycost", 
                   "RTE0"."ps_availqty" AS "ps_availqty"
            FROM partsupp_1 AS "RTE0"("tuid", 
                                      "ps_partkey", 
                                      "ps_suppkey", 
                                      "ps_availqty", 
                                      "ps_supplycost", 
                                      "ps_comment"), 
                 supplier_1 AS "RTE1"("tuid", 
                                      "s_suppkey", 
                                      "s_name", 
                                      "s_address", 
                                      "s_nationkey", 
                                      "s_phone", 
                                      "s_acctbal", 
                                      "s_comment"), 
                 nation_1 AS "RTE2"("tuid", 
                                    "n_nationkey", 
                                    "n_name", 
                                    "n_regionkey", 
                                    "n_comment")
            WHERE ("RTE0"."ps_suppkey") = ("RTE1"."s_suppkey") AND
                  ("RTE1"."s_nationkey") = ("RTE2"."n_nationkey") AND
                  ("RTE2"."n_name") = ('GERMANY')) AS "subquery3"("tuid", 
                                                                  "ps_partkey", 
                                                                  "ps_supplycost", 
                                                                  "ps_availqty")
      GROUP BY ("subquery3"."ps_partkey")
      HAVING (sum(("subquery3"."ps_supplycost") *
                  ("subquery3"."ps_availqty"))) >
             ((SELECT (sum(("subquery7"."ps_supplycost") *
                           ("subquery7"."ps_availqty"))) *
                      (0.0001000000) AS "sum"
               FROM (SELECT writejoin(2, 
                                      "RTE4"."tuid", 
                                      "RTE5"."tuid", 
                                      "RTE6"."tuid") AS "tuid", 
                            "RTE4"."ps_supplycost" AS "ps_supplycost", 
                            "RTE4"."ps_availqty" AS "ps_availqty"
                     FROM partsupp_1 AS "RTE4"("tuid", 
                                               "ps_partkey", 
                                               "ps_suppkey", 
                                               "ps_availqty", 
                                               "ps_supplycost", 
                                               "ps_comment"), 
                          supplier_1 AS "RTE5"("tuid", 
                                               "s_suppkey", 
                                               "s_name", 
                                               "s_address", 
                                               "s_nationkey", 
                                               "s_phone", 
                                               "s_acctbal", 
                                               "s_comment"), 
                          nation_1 AS "RTE6"("tuid", 
                                             "n_nationkey", 
                                             "n_name", 
                                             "n_regionkey", 
                                             "n_comment")
                     WHERE ("RTE4"."ps_suppkey") = ("RTE5"."s_suppkey") AND
                           ("RTE5"."s_nationkey") = ("RTE6"."n_nationkey") AND
                           ("RTE6"."n_name") =
                           ('GERMANY')) AS "subquery7"("tuid", 
                                                        "ps_supplycost", 
                                                        "ps_availqty")
               HAVING True))) AS "subquery8"("tuid", 
                                             "ps_partkey", 
                                             "value")
ORDER BY ("subquery8"."value") DESC
LIMIT NULL;
