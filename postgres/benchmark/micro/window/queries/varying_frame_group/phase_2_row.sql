SELECT "win"."tuid" AS "tuid",
       "RTE0"."id" AS "id",
       rb_build_agg("RTE0"."id") OVER(PARTITION BY ("win"."part")
                                    ORDER BY ("win"."rank") ASC GROUPS BETWEEN
                                    UNBOUNDED PRECEDING AND
                                    CURRENT ROW) AS "sum"
FROM skew_1_0_num_ROW_COUNT_2_row AS "RTE0"("tuid",
                                   "id"),
     LATERAL readwindow(1,
                            "RTE0"."tuid") AS "win"("tuid", "part", "rank");
