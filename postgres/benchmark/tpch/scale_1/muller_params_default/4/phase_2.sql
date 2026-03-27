SELECT "orderby"."tuid" AS "tuid", 
       "subquery3"."o_orderpriority" AS "o_orderpriority", 
       "subquery3"."order_count" AS "order_count"
FROM (SELECT "group"."tuid" AS "tuid", 
             concat_agg("subquery2"."o_orderpriority") AS "o_orderpriority", 
             ARRAY[] :: int4[] AS "order_count"
      FROM (SELECT "join"."tuid" AS "tuid", 
                   "RTE0"."o_orderpriority" AS "o_orderpriority"
            FROM orders_2 AS "RTE0"("tuid", 
                                    "o_orderkey", 
                                    "o_custkey", 
                                    "o_orderstatus", 
                                    "o_totalprice", 
                                    "o_orderdate", 
                                    "o_orderpriority", 
                                    "o_clerk", 
                                    "o_shippriority", 
                                    "o_comment"), 
                 LATERAL readjoin(2, 
                                  "RTE0"."tuid") AS "join"("tuid")) AS "subquery2"("tuid", 
                                                                                   "o_orderpriority"), 
           LATERAL readaggregation(3, 
                                   "subquery2"."tuid") AS "group"("tuid")
      GROUP BY ("group"."tuid")) AS "subquery3"("tuid", 
                                                "o_orderpriority", 
                                                "order_count"), 
     LATERAL readorderby(5, 
                         "subquery3"."tuid") AS "orderby"("tuid");