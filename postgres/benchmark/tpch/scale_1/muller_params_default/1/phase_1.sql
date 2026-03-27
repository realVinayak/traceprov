
-- SQLProv
-- translation date: 21.03.18 18:27:20


-- *** phase 1
SELECT writeorderby(4, 
                    "subquery2"."tuid", 
                    row_number() OVER(ORDER BY l_returnflag ASC, 
                                               l_linestatus ASC RANGE UNBOUNDED PRECEDING)) AS "tuid", 
       "subquery2"."l_returnflag" AS "l_returnflag", 
       "subquery2"."l_linestatus" AS "l_linestatus", 
       "subquery2"."sum_qty" AS "sum_qty", 
       "subquery2"."sum_base_price" AS "sum_base_price", 
       "subquery2"."sum_disc_price" AS "sum_disc_price", 
       "subquery2"."sum_charge" AS "sum_charge", 
       "subquery2"."avg_qty" AS "avg_qty", 
       "subquery2"."avg_price" AS "avg_price", 
       "subquery2"."avg_disc" AS "avg_disc", 
       "subquery2"."count_order" AS "count_order"
FROM (SELECT writeaggregation(2, 
                              array_agg("subquery1"."tuid")) AS "tuid", 
             "subquery1"."l_returnflag" AS "l_returnflag", 
             "subquery1"."l_linestatus" AS "l_linestatus", 
             sum("subquery1"."l_quantity") AS "sum_qty", 
             sum("subquery1"."l_extendedprice") AS "sum_base_price", 
             sum("subquery1"."l_extendedprice"
                 *
                 (1 - "subquery1"."l_discount")) AS "sum_disc_price", 
             sum(("subquery1"."l_extendedprice"
                  *
                  (1 - "subquery1"."l_discount"))
                 *
                 (1 + "subquery1"."l_tax")) AS "sum_charge", 
             avg("subquery1"."l_quantity") AS "avg_qty", 
             avg("subquery1"."l_extendedprice") AS "avg_price", 
             avg("subquery1"."l_discount") AS "avg_disc", 
             count(*) AS "count_order"
      FROM (SELECT writejoin(1, "RTE0"."tuid") AS "tuid", 
                   "RTE0"."l_returnflag" AS "l_returnflag", 
                   "RTE0"."l_linestatus" AS "l_linestatus", 
                   "RTE0"."l_quantity" AS "l_quantity", 
                   "RTE0"."l_extendedprice" AS "l_extendedprice", 
                   "RTE0"."l_discount" AS "l_discount", 
                   "RTE0"."l_tax" AS "l_tax"
            FROM lineitem_1 AS "RTE0"("tuid", 
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
            WHERE (date_part('epoch', "RTE0"."l_shipdate"))
                  <=
                  904694400) AS "subquery1"("tuid", 
                                            "l_returnflag", 
                                            "l_linestatus", 
                                            "l_quantity", 
                                            "l_extendedprice", 
                                            "l_discount", 
                                            "l_tax")
      GROUP BY l_returnflag, 
               l_linestatus) AS "subquery2"("tuid", 
                                            "l_returnflag", 
                                            "l_linestatus", 
                                            "sum_qty", 
                                            "sum_base_price", 
                                            "sum_disc_price", 
                                            "sum_charge", 
                                            "avg_qty", 
                                            "avg_price", 
                                            "avg_disc", 
                                            "count_order")
ORDER BY l_returnflag ASC, l_linestatus ASC
LIMIT NULL;

