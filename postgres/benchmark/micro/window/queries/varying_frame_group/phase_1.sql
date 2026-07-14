SELECT writewindow(1,
                       "RTE0"."tuid",
                       first_value("RTE0"."tuid") OVER(ORDER BY ("RTE0"."z") ASC GROUPS BETWEEN
                                                       UNBOUNDED PRECEDING AND
                                                       CURRENT ROW),
                       rank() OVER(ORDER BY ("RTE0"."z") ASC GROUPS BETWEEN
                                   UNBOUNDED PRECEDING AND
                                   CURRENT ROW)) AS "tuid",
       "RTE0"."id" AS "id",
       sum("RTE0"."id") OVER(ORDER BY ("RTE0"."z") ASC GROUPS BETWEEN
                             UNBOUNDED PRECEDING AND CURRENT ROW) AS "sum"
FROM skew_1_0_num_ROW_COUNT_1 AS "RTE0"("tuid",
                                   "id",
                                   "z",
                                   "val",
                                   "group_1",
                                   "group_4",
                                   "group_32",
                                   "group_128")