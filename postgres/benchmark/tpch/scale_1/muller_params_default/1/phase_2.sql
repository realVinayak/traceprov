
-- SQLProv
-- translation date: 21.03.18 18:27:20


-- *** phase 2 without Y
SELECT "orderby"."tuid" AS "tuid", 
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
FROM (SELECT "group"."tuid" AS "tuid", 
             concat_agg("subquery1"."l_returnflag") AS "l_returnflag", 
             concat_agg("subquery1"."l_linestatus") AS "l_linestatus", 
             concat_agg("subquery1"."l_quantity") AS "sum_qty", 
             concat_agg("subquery1"."l_extendedprice") AS "sum_base_price", 
             concat_agg("subquery1"."l_extendedprice"
                        ||
                        ((ARRAY[] :: int4[])
                         ||
                         "subquery1"."l_discount")) AS "sum_disc_price", 
             concat_agg(("subquery1"."l_extendedprice"
                         ||
                         ((ARRAY[] :: int4[]) || "subquery1"."l_discount"))
                        ||
                        ((ARRAY[] :: int4[])
                         ||
                         "subquery1"."l_tax")) AS "sum_charge", 
             concat_agg("subquery1"."l_quantity") AS "avg_qty", 
             concat_agg("subquery1"."l_extendedprice") AS "avg_price", 
             concat_agg("subquery1"."l_discount") AS "avg_disc", 
             ARRAY[] :: int4[] AS "count_order"
      FROM (SELECT "join"."tuid" AS "tuid", 
                   "RTE0"."l_returnflag" AS "l_returnflag", 
                   "RTE0"."l_linestatus" AS "l_linestatus", 
                   "RTE0"."l_quantity" AS "l_quantity", 
                   "RTE0"."l_extendedprice" AS "l_extendedprice", 
                   "RTE0"."l_discount" AS "l_discount", 
                   "RTE0"."l_tax" AS "l_tax"
            FROM lineitem_2 AS "RTE0"("tuid", 
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
                 LATERAL readjoin(1, 
                                  "RTE0"."tuid") AS "join"("tuid")) AS "subquery1"("tuid", 
                                                                                   "l_returnflag", 
                                                                                   "l_linestatus", 
                                                                                   "l_quantity", 
                                                                                   "l_extendedprice", 
                                                                                   "l_discount", 
                                                                                   "l_tax"), 
           LATERAL readaggregation(2, 
                                   "subquery1"."tuid") AS "group"("tuid")
      GROUP BY "group".tuid) AS "subquery2"("tuid", 
                                    "l_returnflag", 
                                    "l_linestatus", 
                                    "sum_qty", 
                                    "sum_base_price", 
                                    "sum_disc_price", 
                                    "sum_charge", 
                                    "avg_qty", 
                                    "avg_price", 
                                    "avg_disc", 
                                    "count_order"), 
     LATERAL readorderby(4, 
                         "subquery2"."tuid") AS "orderby"("tuid");

