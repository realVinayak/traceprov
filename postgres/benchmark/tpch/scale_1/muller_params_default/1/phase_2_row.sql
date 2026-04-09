-- SQLProv
-- translation date: 21.03.18 18:27:20
-- *** phase 2 without Y
SELECT "orderby"."tuid" AS "tuid",
       "subquery2"."l_orderkey" as "l_orderkey"
FROM (
              SELECT "group"."tuid" AS "tuid",
                     concat_agg("subquery1"."l_orderkey") AS "l_orderkey"
              FROM (
                            SELECT "join"."tuid" AS "tuid",
                                   "RTE0"."l_orderkey" AS "l_orderkey"
                            FROM lineitem_2_row AS "RTE0"(
                                          "tuid",
                                          "l_orderkey"
                                   ),
                                   LATERAL readjoin(
                                          1,
                                          "RTE0"."tuid"
                                   ) AS "join"("tuid")
                     ) AS "subquery1"(
                            "tuid",
                            "l_orderkey"
                     ),
                     LATERAL readaggregation(
                            2,
                            "subquery1"."tuid"
                     ) AS "group"("tuid")
              GROUP BY "group".tuid
       ) AS "subquery2"(
              "tuid",
              "l_orderkey"
       ),
       LATERAL readorderby(4, "subquery2"."tuid") AS "orderby"("tuid");