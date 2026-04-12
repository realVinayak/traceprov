-- PgLlProvenance
-- translation date: 08.02.18 14:04:39
-- *** phase 2 without Y
SELECT "orderby"."tuid" AS "tuid",
       "subquery4"."c_custkey" AS "c_custkey"
FROM (
              SELECT "group"."tuid" AS "tuid",
                     concat_agg("subquery3"."c_custkey") AS "c_custkey"
              FROM (
                            SELECT "join"."tuid" AS "tuid",
                                   "RTE0"."c_custkey" AS "c_custkey"
                            FROM customer_2_row AS "RTE0"(
                                          "tuid",
                                          "c_custkey"
                                   ),
                                   LATERAL readjoin(
                                          3,
                                          "RTE0"."tuid"
                                   ) AS "join"("tuid")
                     ) AS "subquery3"(
                            "tuid",
                            "c_custkey"
                     ),
                     LATERAL readaggregation(
                            4,
                            "subquery3"."tuid"
                     ) AS "group"("tuid")
              GROUP BY ("group"."tuid")
       ) AS "subquery4"(
              "tuid",
              "c_custkey"
       ),
       LATERAL readorderby(6, "subquery4"."tuid") AS "orderby"("tuid");