copy (
    SELECT
        combined_entry.column_0::bigint,
        combined_entry.column_1::bigint,
        combined_entry.column_2::bigint
    FROM
        traceprov_read_worker_layer (1::bigint, 5::bigint) AS combined_entry
) to 'q1_combine_layer5.csv';