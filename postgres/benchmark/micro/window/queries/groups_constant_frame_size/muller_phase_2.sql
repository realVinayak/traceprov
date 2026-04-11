SELECT "win"."tuid" AS "tuid",
    concat_agg("RTE0"."id") OVER(
        PARTITION BY ("win"."part")
        ORDER BY ("win"."rank") ASC GROUPS BETWEEN 3 PRECEDING AND 2 FOLLOWING
    ) AS "max"
FROM skew_1_0_num_NUM_2_row AS "RTE0"("tuid", "id"),
    LATERAL readwindow(1, "RTE0"."tuid") AS "win"("tuid", "part", "rank");