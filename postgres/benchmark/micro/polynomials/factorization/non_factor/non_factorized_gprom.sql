WITH temp_view_1 AS (
    SELECT
        /*+ materialize */
        F0_0.b AS b,
        F0_0."b1" AS "b1",
        F1_0.b AS "b2",
        F0_0."prov_polynomial__table__0_id" AS "prov_polynomial__table__0_id",
        F0_0."prov_polynomial__table__1_id" AS "prov_polynomial__table__1_id",
        F1_0."prov_polynomial__table__2_id" AS "prov_polynomial__table__2_id"
    FROM
        (
            (
                SELECT
                    F0_0.b AS b,
                    F1_0.b AS "b1",
                    F0_0."prov_polynomial__table__0_id" AS "prov_polynomial__table__0_id",
                    F1_0."prov_polynomial__table__1_id" AS "prov_polynomial__table__1_id"
                FROM
                    (
                        (
                            SELECT
                                F0_0.b AS b,
                                F0_0.id AS "prov_polynomial__table__0_id"
                            FROM
                                "polynomial_table_0" F0_0
                            where
                                id <= NUM
                        ) F0_0
                        CROSS JOIN (
                            SELECT
                                F0_0.b AS b,
                                F0_0.id AS "prov_polynomial__table__1_id"
                            FROM
                                "polynomial_table_1" F0_0
                            where
                                id <= NUM
                        ) F1_0
                    )
            ) F0_0
            CROSS JOIN (
                SELECT
                    F0_0.b AS b,
                    F0_0.id AS "prov_polynomial__table__2_id"
                FROM
                    "polynomial_table_2" F0_0
                where
                    id <= NUM
            ) F1_0
        )
),
temp_view_0 AS (
    SELECT
        /*+ materialize */
        1 AS "1",
        F0_0."prov_polynomial__table__0_id" AS "prov_polynomial__table__0_id",
        F0_0."prov_polynomial__table__1_id" AS "prov_polynomial__table__1_id",
        F0_0."prov_polynomial__table__2_id" AS "prov_polynomial__table__2_id"
    FROM
        (
            SELECT
                *
            FROM
                temp_view_1
        ) F0_0
)
SELECT
    F0_0."1" AS "1",
    string_agg(
        (
            (
                (
                    (
                        '(' || (
                            (
                                (
                                    ('(' || F0_0."prov_polynomial__table__0_id") || ' ⊗ '
                                ) || F0_0."prov_polynomial__table__1_id"
                            ) || ')'
                        )
                    ) || ' ⊗ '
                ) || F0_0."prov_polynomial__table__2_id"
            ) || ')'
        ),
        ' + '
    ) AS "PROV"
FROM
    (
        SELECT
            *
        FROM
            temp_view_0
    ) F0_0
GROUP BY
    F0_0."1";