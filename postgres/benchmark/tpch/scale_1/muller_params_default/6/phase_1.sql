SELECT writeaggregation(2, 
                        array_agg("subquery1"."tuid")) AS "tuid", 
       sum(("subquery1"."l_extendedprice") *
           ("subquery1"."l_discount")) AS "revenue"
FROM (SELECT writejoin(1, "RTE0"."tuid") AS "tuid", 
             "RTE0"."l_extendedprice" AS "l_extendedprice", 
             "RTE0"."l_discount" AS "l_discount"
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
      WHERE (date_part('epoch', "RTE0"."l_shipdate")) >=
            (757378800) AND (date_part('epoch', 
                                       "RTE0"."l_shipdate")) < (788914800) AND
            ("RTE0"."l_discount") >= ((0.06) -
                                      (0.01)) AND
            ("RTE0"."l_discount") <= ((0.06) +
                                      (0.01)) AND
            ("RTE0"."l_quantity") < (24)) AS "subquery1"("tuid", 
                                                         "l_extendedprice", 
                                                         "l_discount")
HAVING True;
