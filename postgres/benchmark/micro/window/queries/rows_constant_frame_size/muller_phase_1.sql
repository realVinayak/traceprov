SELECT writewindow(
        1,
        "RTE0"."tuid",
        first_value("RTE0"."tuid") OVER(
            ORDER BY "RTE0"."z" ASC ROWS BETWEEN 3 PRECEDING AND 2 FOLLOWING
        ),
        rank() OVER(
            ORDER BY "RTE0"."z" ASC ROWS BETWEEN 3 PRECEDING AND 2 FOLLOWING
        )
    ) AS "tuid",
    "RTE0"."id" AS "id",
    "RTE0"."z" AS "z",
    "RTE0"."val" AS "val",
    max("RTE0"."id" + "RTE0"."z") OVER(
        ORDER BY "RTE0"."z" ASC ROWS BETWEEN 3 PRECEDING AND 2 FOLLOWING
    ) AS "max"
FROM skew_1_0_num_NUM_1 AS "RTE0"(
        "tuid",
        "id",
        "z",
        "val"
    );