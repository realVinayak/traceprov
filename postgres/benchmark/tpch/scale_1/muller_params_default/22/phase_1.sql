
-- PgLlProvenance
-- translation date: 08.02.18 14:04:39


-- *** phase 1
SELECT writeorderby(6, 
                    "subquery4"."tuid", 
                    row_number() OVER(ORDER BY ("subquery4"."cntrycode") ASC RANGE UNBOUNDED PRECEDING)) AS "tuid", 
       "subquery4"."cntrycode" AS "cntrycode", 
       "subquery4"."numcust" AS "numcust", 
       "subquery4"."totacctbal" AS "totacctbal"
FROM (SELECT writeaggregation(4, 
                              array_agg("subquery3"."tuid")) AS "tuid", 
             "subquery3"."cntrycode" AS "cntrycode", 
             count(*) AS "numcust", 
             sum("subquery3"."c_acctbal") AS "totacctbal"
      FROM (SELECT writejoin(3, "RTE0"."tuid") AS "tuid", 
                   substring("RTE0"."c_phone", 1, 2) AS "cntrycode", 
                   "RTE0"."c_acctbal" AS "c_acctbal"
            FROM customer_1 AS "RTE0"("tuid", 
                                      "c_custkey", 
                                      "c_name", 
                                      "c_address", 
                                      "c_nationkey", 
                                      "c_phone", 
                                      "c_acctbal", 
                                      "c_mktsegment", 
                                      "c_comment")
            WHERE (substring("RTE0"."c_phone", 1, 2) =
                   ANY(ARRAY['13', 
                             '31', 
                             '23', 
                             '29', 
                             '30', 
                             '18', 
                             '17'] :: text[]) AND ("RTE0"."c_acctbal") >
                                                  ((SELECT avg("RTE1"."c_acctbal") AS "avg"
                                                    FROM customer_1 AS "RTE1"("tuid", 
                                                                              "c_custkey", 
                                                                              "c_name", 
                                                                              "c_address", 
                                                                              "c_nationkey", 
                                                                              "c_phone", 
                                                                              "c_acctbal", 
                                                                              "c_mktsegment", 
                                                                              "c_comment")
                                                    WHERE (("RTE1"."c_acctbal")
                                                           > (0.0) AND
                                                           substring("RTE1"."c_phone", 
                                                                     1, 
                                                                     2) =
                                                           ANY(ARRAY['13', 
                                                                     '31', 
                                                                     '23', 
                                                                     '29', 
                                                                     '30', 
                                                                     '18', 
                                                                     '17'] :: text[]))))
                   AND
                   NOT EXISTS(SELECT "RTE2"."o_orderkey" AS "o_orderkey", 
                                     "RTE2"."o_custkey" AS "o_custkey", 
                                     "RTE2"."o_orderstatus" AS "o_orderstatus", 
                                     "RTE2"."o_totalprice" AS "o_totalprice", 
                                     "RTE2"."o_orderdate" AS "o_orderdate", 
                                     "RTE2"."o_orderpriority" AS "o_orderpriority", 
                                     "RTE2"."o_clerk" AS "o_clerk", 
                                     "RTE2"."o_shippriority" AS "o_shippriority", 
                                     "RTE2"."o_comment" AS "o_comment"
                              FROM orders_1 AS "RTE2"("tuid", 
                                                      "o_orderkey", 
                                                      "o_custkey", 
                                                      "o_orderstatus", 
                                                      "o_totalprice", 
                                                      "o_orderdate", 
                                                      "o_orderpriority", 
                                                      "o_clerk", 
                                                      "o_shippriority", 
                                                      "o_comment")
                              WHERE ("RTE2"."o_custkey") =
                                    ("RTE0"."c_custkey")))) AS "subquery3"("tuid", 
                                                                           "cntrycode", 
                                                                           "c_acctbal")
      GROUP BY ("subquery3"."cntrycode")
      HAVING True) AS "subquery4"("tuid", 
                                  "cntrycode", 
                                  "numcust", 
                                  "totacctbal")
ORDER BY ("subquery4"."cntrycode") ASC
LIMIT NULL;

