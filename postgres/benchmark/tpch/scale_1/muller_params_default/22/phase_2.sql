
-- PgLlProvenance
-- translation date: 08.02.18 14:04:39


-- *** phase 2 without Y
SELECT "orderby"."tuid" AS "tuid", 
       "subquery4"."cntrycode" AS "cntrycode", 
       "subquery4"."numcust" AS "numcust", 
       "subquery4"."totacctbal" AS "totacctbal"
FROM (SELECT "group"."tuid" AS "tuid", 
             concat_agg("subquery3"."cntrycode") AS "cntrycode", 
             ARRAY[] :: int4[] AS "numcust", 
             concat_agg("subquery3"."c_acctbal") AS "totacctbal"
      FROM (SELECT "join"."tuid" AS "tuid", 
                   "RTE0"."c_phone" AS "cntrycode", 
                   "RTE0"."c_acctbal" AS "c_acctbal"
            FROM customer_2 AS "RTE0"("tuid", 
                                      "c_custkey", 
                                      "c_name", 
                                      "c_address", 
                                      "c_nationkey", 
                                      "c_phone", 
                                      "c_acctbal", 
                                      "c_mktsegment", 
                                      "c_comment"), 
                 LATERAL readjoin(3, 
                                  "RTE0"."tuid") AS "join"("tuid")) AS "subquery3"("tuid", 
                                                                                   "cntrycode", 
                                                                                   "c_acctbal"), 
           LATERAL readaggregation(4, 
                                   "subquery3"."tuid") AS "group"("tuid")
      GROUP BY ("group"."tuid")) AS "subquery4"("tuid", 
                                                "cntrycode", 
                                                "numcust", 
                                                "totacctbal"), 
     LATERAL readorderby(6, 
                         "subquery4"."tuid") AS "orderby"("tuid");

