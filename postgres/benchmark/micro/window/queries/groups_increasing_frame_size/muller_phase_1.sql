SELECT writewindow(
        1,
        "RTE0"."tuid",
        first_value("RTE0"."tuid") OVER(
            ORDER BY "RTE0"."z" ASC GROUPS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW
        ),
        rank() OVER(
            ORDER BY "RTE0"."z" ASC GROUPS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW
        )
    ) AS "tuid",
    "RTE0"."id" AS "id",
    "RTE0"."z" AS "z",
    "RTE0"."val" AS "val",
    count("RTE0"."id") OVER(
        ORDER BY "RTE0"."z" ASC GROUPS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW
    ) AS "count"
FROM skew_1_0_num_NUM_1 AS "RTE0"(
        "tuid",
        "id",
        "z",
        "val"
    );