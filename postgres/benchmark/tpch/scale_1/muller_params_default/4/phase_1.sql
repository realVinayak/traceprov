SELECT writeorderby(5, 
                    "subquery3"."tuid", 
                    row_number() OVER(ORDER BY ("subquery3"."o_orderpriority") ASC RANGE UNBOUNDED PRECEDING)) AS "tuid", 
       "subquery3"."o_orderpriority" AS "o_orderpriority", 
       "subquery3"."order_count" AS "order_count"
FROM (SELECT writeaggregation(3, 
                              array_agg("subquery2"."tuid")) AS "tuid", 
             "subquery2"."o_orderpriority" AS "o_orderpriority", 
             count(*) AS "order_count"
      FROM (SELECT writejoin(2, "RTE0"."tuid") AS "tuid", 
                   "RTE0"."o_orderpriority" AS "o_orderpriority"
            FROM orders_1 AS "RTE0"("tuid", 
                                    "o_orderkey", 
                                    "o_custkey", 
                                    "o_orderstatus", 
                                    "o_totalprice", 
                                    "o_orderdate", 
                                    "o_orderpriority", 
                                    "o_clerk", 
                                    "o_shippriority", 
                                    "o_comment")
            WHERE (date_part('epoch', "RTE0"."o_orderdate")) >=
                  (741484800) AND (date_part('epoch', 
                                             "RTE0"."o_orderdate")) <
                                  (749260800) AND
                  EXISTS(SELECT "RTE1"."l_orderkey" AS "l_orderkey", 
                                "RTE1"."l_partkey" AS "l_partkey", 
                                "RTE1"."l_suppkey" AS "l_suppkey", 
                                "RTE1"."l_linenumber" AS "l_linenumber", 
                                "RTE1"."l_quantity" AS "l_quantity", 
                                "RTE1"."l_extendedprice" AS "l_extendedprice", 
                                "RTE1"."l_discount" AS "l_discount", 
                                "RTE1"."l_tax" AS "l_tax", 
                                "RTE1"."l_returnflag" AS "l_returnflag", 
                                "RTE1"."l_linestatus" AS "l_linestatus", 
                                "RTE1"."l_shipdate" AS "l_shipdate", 
                                "RTE1"."l_commitdate" AS "l_commitdate", 
                                "RTE1"."l_receiptdate" AS "l_receiptdate", 
                                "RTE1"."l_shipinstruct" AS "l_shipinstruct", 
                                "RTE1"."l_shipmode" AS "l_shipmode", 
                                "RTE1"."l_comment" AS "l_comment"
                         FROM lineitem_1 AS "RTE1"("tuid", 
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
                                                   "l_comment")
                         WHERE ("RTE1"."l_orderkey") = ("RTE0"."o_orderkey") AND
                               ("RTE1"."l_commitdate") <
                               ("RTE1"."l_receiptdate"))) AS "subquery2"("tuid", 
                                                                         "o_orderpriority")
      GROUP BY ("subquery2"."o_orderpriority")
      HAVING True) AS "subquery3"("tuid", 
                                  "o_orderpriority", 
                                  "order_count")
ORDER BY ("subquery3"."o_orderpriority") ASC
LIMIT NULL;