DROP TABLE IF EXISTS data_table_%NUM_ROWS%;

CREATE TABLE data_table_%NUM_ROWS% (
    id serial primary key,
    min_value BIGINT,
    negative_group_number INT,
    group_number INT
);