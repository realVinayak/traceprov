SELECT writeorderby(4, 
                    "subquery7"."tuid", 
                    row_number() OVER(ORDER BY ("subquery7"."revenue") DESC RANGE UNBOUNDED PRECEDING)) AS "tuid", 
       "subquery7"."n_name" AS "n_name", 
       "subquery7"."revenue" AS "revenue"
FROM (SELECT writeaggregation(2, 
                              array_agg("subquery6"."tuid")) AS "tuid", 
             "subquery6"."n_name" AS "n_name", 
             sum(("subquery6"."l_extendedprice") * ((1) -
                                                    ("subquery6"."l_discount"))) AS "revenue"
      FROM (SELECT writejoin(1, 
                             "RTE0"."tuid", 
                             "RTE1"."tuid", 
                             "RTE2"."tuid", 
                             "RTE3"."tuid", 
                             "RTE4"."tuid", 
                             "RTE5"."tuid") AS "tuid", 
                   "RTE4"."n_name" AS "n_name", 
                   "RTE2"."l_extendedprice" AS "l_extendedprice", 
                   "RTE2"."l_discount" AS "l_discount"
            FROM customer_1 AS "RTE0"("tuid", 
                                      "c_custkey", 
                                      "c_name", 
                                      "c_address", 
                                      "c_nationkey", 
                                      "c_phone", 
                                      "c_acctbal", 
                                      "c_mktsegment", 
                                      "c_comment"), 
                 orders_1 AS "RTE1"("tuid", 
                                    "o_orderkey", 
                                    "o_custkey", 
                                    "o_orderstatus", 
                                    "o_totalprice", 
                                    "o_orderdate", 
                                    "o_orderpriority", 
                                    "o_clerk", 
                                    "o_shippriority", 
                                    "o_comment"), 
                 lineitem_1 AS "RTE2"("tuid", 
                                      "l_orderkey", 
                                      "l_partkey", 
                                      "l_suppkey", 
                                      "l_linenumber", 
                                      "l_quantity", 
                                      "l_extendedprice", 
                                      "l_discount", 
                                      "l_tax", 
                                      "l_returnflag", 
                                      "l_linestatus", 
                                      "l_shipdate", 
                                      "l_commitdate", 
                                      "l_receiptdate", 
                                      "l_shipinstruct", 
                                      "l_shipmode", 
                                      "l_comment"), 
                 supplier_1 AS "RTE3"("tuid", 
                                      "s_suppkey", 
                                      "s_name", 
                                      "s_address", 
                                      "s_nationkey", 
                                      "s_phone", 
                                      "s_acctbal", 
                                      "s_comment"), 
                 nation_1 AS "RTE4"("tuid", 
                                    "n_nationkey", 
                                    "n_name", 
                                    "n_regionkey", 
                                    "n_comment"), 
                 region_1 AS "RTE5"("tuid", 
                                    "r_regionkey", 
                                    "r_name", 
                                    "r_comment")
            WHERE ("RTE0"."c_custkey") = ("RTE1"."o_custkey") AND
                  ("RTE2"."l_orderkey") = ("RTE1"."o_orderkey") AND
                  ("RTE2"."l_suppkey") = ("RTE3"."s_suppkey") AND
                  ("RTE0"."c_nationkey") = ("RTE3"."s_nationkey") AND
                  ("RTE3"."s_nationkey") = ("RTE4"."n_nationkey") AND
                  ("RTE4"."n_regionkey") = ("RTE5"."r_regionkey") AND
                  ("RTE5"."r_name") = ('ASIA') AND (date_part('epoch', 
                                                              "RTE1"."o_orderdate"))
                                                   >= (757378800) AND
                  (date_part('epoch', "RTE1"."o_orderdate")) <
                  (788914800)) AS "subquery6"("tuid", 
                                              "n_name", 
                                              "l_extendedprice", 
                                              "l_discount")
      GROUP BY ("subquery6"."n_name")
      HAVING True) AS "subquery7"("tuid", "n_name", "revenue")
ORDER BY ("subquery7"."revenue") DESC
LIMIT NULL;