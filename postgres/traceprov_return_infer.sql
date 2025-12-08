DROP FUNCTION IF EXISTS traceprov_infer (integer, integer, integer);

DROP FUNCTION IF EXISTS traceprov_infer_time (integer, integer, integer);

DROP FUNCTION IF EXISTS traceprov_sync_time (integer);

DROP FUNCTION IF EXISTS traceprov_layer_stat (integer, integer);

DROP FUNCTION IF EXISTS traceprov_layer_stat ();

CREATE
OR REPLACE FUNCTION traceprov_infer (
    IN integer,
    IN integer,
    IN integer,
    OUT f1 integer,
    OUT f2 integer
) RETURNS SETOF record AS '$libdir/__FILE__',
'traceprov_infer' LANGUAGE C STRICT PARALLEL SAFE;

CREATE
OR REPLACE FUNCTION traceprov_infer_time (IN integer, IN integer, IN integer) RETURNS BIGINT AS '$libdir/__FILE__',
'traceprov_infer_time' LANGUAGE C STRICT PARALLEL SAFE;

CREATE
OR REPLACE FUNCTION traceprov_sync_time (IN integer) RETURNS BIGINT AS '$libdir/__FILE__',
'traceprov_sync_time' LANGUAGE C STRICT PARALLEL SAFE;

CREATE
OR REPLACE FUNCTION traceprov_layer_stat (
    OUT is_leader_layer INT,
    OUT worker_id INT,
    OUT layer_id INT,
    OUT num_pk_records INT,
    OUT layer_size INT,
    OUT num_groups INT,
    OUT layer_number INT,
    OUT record_padding INT,
    OUT layer_fd INT,
    OUT logged_record_count BIGINT,
    OUT sorted_by_group INT,
    OUT aggregate_strategy INT,
    OUT hash_buckets text,
    OUT combined_aggregate_layer_number INT,
    OUT rows_layer_number INT
) RETURNS SETOF record AS '$libdir/__FILE__',
'traceprov_layer_stat' LANGUAGE C STRICT PARALLEL SAFE;

CREATE
OR REPLACE FUNCTION traceprov_infer_graph () RETURNS BIGINT AS '$libdir/__FILE__',
'traceprov_infer_graph' LANGUAGE C STRICT PARALLEL SAFE;

CREATE
OR REPLACE FUNCTION traceprov_json_graph () RETURNS text AS '$libdir/__FILE__',
'traceprov_json_graph' LANGUAGE C STRICT PARALLEL SAFE;