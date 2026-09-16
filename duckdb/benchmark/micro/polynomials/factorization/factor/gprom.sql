WITH temp_view_1 AS (
    SELECT
        /*+ materialize */
        F0_0.b AS b,
        F0_0.rowid AS "prov_polynomial__table__0_id"
    FROM
        "polynomial_table_0" F0_0
    where
        id <= NUM
),
temp_view_0 AS (
    SELECT
        /*+ materialize */
        F0_0.b AS b,
        F0_0."prov_polynomial__table__0_id" AS "prov_polynomial__table__0_id"
    FROM
        (
            SELECT
                *
            FROM
                temp_view_1
        ) F0_0
),
temp_view_3 AS (
    SELECT
        /*+ materialize */
        F0_0.b AS b,
        F0_0.rowid AS "prov_polynomial__table__1_id"
    FROM
        "polynomial_table_1" F0_0
    where
        id <= NUM
),
temp_view_2 AS (
    SELECT
        /*+ materialize */
        F0_0.b AS b,
        F0_0."prov_polynomial__table__1_id" AS "prov_polynomial__table__1_id"
    FROM
        (
            SELECT
                *
            FROM
                temp_view_3
        ) F0_0
),
temp_view_5 AS (
    SELECT
        /*+ materialize */
        F0_0.b AS b,
        F0_0.rowid AS "prov_polynomial__table__2_id"
    FROM
        "polynomial_table_2" F0_0
    where
        id <= NUM
),
temp_view_4 AS (
    SELECT
        /*+ materialize */
        F0_0.b AS b,
        F0_0."prov_polynomial__table__2_id" AS "prov_polynomial__table__2_id"
    FROM
        (
            SELECT
                *
            FROM
                temp_view_5
        ) F0_0
)
SELECT
    (
        (((F0_0."PROV" || ' ⊗ ') || F1_0."PROV") || ' ⊗ ') || F2_0."PROV"
    ) AS "polynomial"
FROM
    (
        (
            (
                SELECT
                    F0_0.b AS b,
                    '(' || string_agg(
                        F0_0."prov_polynomial__table__0_id" :: text,
                        ' ⊕ '
                    ) || ')' AS "PROV"
                FROM
                    (
                        SELECT
                            *
                        FROM
                            temp_view_0
                    ) F0_0
                GROUP BY
                    F0_0.b
            ) F0_0
            JOIN (
                SELECT
                    F0_0.b AS b,
                    '(' || string_agg(
                        F0_0."prov_polynomial__table__1_id" :: text,
                        ' ⊕ '
                    ) || ')' AS "PROV"
                FROM
                    (
                        SELECT
                            *
                        FROM
                            temp_view_2
                    ) F0_0
                GROUP BY
                    F0_0.b
            ) F1_0 ON ((F0_0.b = F1_0.b))
        )
        JOIN (
            SELECT
                F0_0.b AS b,
                '(' || string_agg(
                    F0_0."prov_polynomial__table__2_id" :: text,
                    ' ⊕ '
                ) || ')' AS "PROV"
            FROM
                (
                    SELECT
                        *
                    FROM
                        temp_view_4
                ) F0_0
            GROUP BY
                F0_0.b
        ) F2_0 ON ((F2_0.b = F1_0.b))
    );