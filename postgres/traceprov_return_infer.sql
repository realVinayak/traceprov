-- TODO: Replace most of the __FILE__ with native PG.
-- When I initially wrote this, I didn't know how easy PG makes to compile and auto load extensions
-- So end up reinventing a lot of the wheel. Replace it.
DROP FUNCTION IF EXISTS traceprov_infer (integer, integer, integer);
DROP FUNCTION IF EXISTS traceprov_infer_time (integer, integer, integer);
DROP FUNCTION IF EXISTS traceprov_sync_time (integer);
DROP FUNCTION IF EXISTS traceprov_layer_stat (integer, integer);
DROP FUNCTION IF EXISTS traceprov_layer_stat ();
DROP FUNCTION IF EXISTS traceprov_perform_derivation ();
DROP FUNCTION IF EXISTS traceprov_perform_derivation (BIGINT);
DROP FUNCTION IF EXISTS traceprov_perform_derivation (BIGINT, BOOLEAN);
DROP FUNCTION IF EXISTS traceprov_perform_generic_derivation (INT, BIGINT);
DROP FUNCTION IF EXISTS traceprov_perform_generic_derivation (BIGINT);
DROP FUNCTION IF EXISTS traceprov_dump_derivation ();
DROP FUNCTION IF EXISTS traceprov_derivation_spec ();
DROP FUNCTION IF EXISTS traceprov_get_sql_derivation ();
DROP FUNCTION IF EXISTS traceprov_infer_graph();
DROP FUNCTION IF EXISTS traceprov_json_graph();
DROP FUNCTION IF EXISTS traceprov_parsed_back();
DROP FUNCTION IF EXISTS traceprov_perform_derivation(IN BIGINT, IN BOOLEAN);
DROP FUNCTION IF EXISTS traceprov_perf_read (INT, BIGINT);
drop function if EXISTS traceprov_perform_duckdb_inference_fast (BIGINT);
drop function if EXISTS traceprov_get_generic_derivation_spec (BOOLEAN);
DROP FUNCTION IF EXISTS traceprov_prepare_for_scan();
DROP FUNCTION IF EXISTS traceprov_run_duckdb_query(cstring);
DROP function if EXISTS traceprov_tableam_handler(internal) CASCADE;
DROP ACCESS METHOD IF EXISTS traceprov_am CASCADE;
drop function if EXISTS traceprov_get_infer_stat (BIGINT, BOOLEAN);
DROP FUNCTION IF EXISTS traceprov_get_total_layer_size ();
-----------------------------------
CREATE FUNCTION traceprov_infer_time (IN integer, IN integer, IN integer) RETURNS BIGINT AS '$libdir/__FILE__',
'traceprov_infer_time' LANGUAGE C STRICT PARALLEL SAFE;
CREATE FUNCTION traceprov_sync_time (IN integer) RETURNS BIGINT AS '$libdir/__FILE__',
'traceprov_sync_time' LANGUAGE C STRICT PARALLEL SAFE;
CREATE FUNCTION traceprov_layer_stat (
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
    OUT rows_layer_number INT,
    OUT null_map_layer_number INT,
    OUT null_map BIGINT,
    OUT last_allocation_size BIGINT,
    OUT initial_allocation_size BIGINT
) RETURNS SETOF record AS '$libdir/__FILE__',
'traceprov_layer_stat' LANGUAGE C STRICT PARALLEL SAFE;
CREATE FUNCTION traceprov_infer_graph () RETURNS BIGINT AS '$libdir/__FILE__',
'traceprov_infer_graph' LANGUAGE C STRICT PARALLEL SAFE;
CREATE FUNCTION traceprov_json_graph () RETURNS text AS '$libdir/__FILE__',
'traceprov_json_graph' LANGUAGE C STRICT PARALLEL SAFE;
CREATE FUNCTION traceprov_parsed_back () RETURNS text AS '$libdir/__FILE__',
'traceprov_parsed_back' LANGUAGE C STRICT PARALLEL SAFE;
CREATE FUNCTION traceprov_perform_derivation (IN BIGINT, IN BOOLEAN) RETURNS SETOF record AS '$libdir/__FILE__',
'traceprov_perform_derivation' LANGUAGE C STRICT PARALLEL SAFE STABLE;
CREATE FUNCTION traceprov_dump_derivation () RETURNS BIGINT AS '$libdir/__FILE__',
'traceprov_dump_derivation' LANGUAGE C STRICT PARALLEL SAFE;
CREATE FUNCTION traceprov_derivation_spec () RETURNS text AS '$libdir/__FILE__',
'traceprov_derivation_spec' LANGUAGE C STRICT PARALLEL SAFE;
CREATE FUNCTION traceprov_get_sql_derivation () RETURNS text AS '$libdir/__FILE__',
'traceprov_get_sql_derivation' LANGUAGE C STRICT PARALLEL SAFE;
CREATE FUNCTION traceprov_perf_read (INT, BIGINT) RETURNS text AS '$libdir/__FILE__',
'traceprov_perf_read' LANGUAGE C STRICT PARALLEL SAFE;
CREATE FUNCTION traceprov_perform_generic_derivation (BIGINT) RETURNS SETOF record AS '$libdir/__FILE__',
'traceprov_perform_generic_derivation' LANGUAGE C STRICT PARALLEL SAFE;
CREATE FUNCTION traceprov_perform_duckdb_inference_fast (BIGINT) RETURNS SETOF record AS '$libdir/__FILE__',
'traceprov_perform_duckdb_inference_fast' LANGUAGE C STRICT PARALLEL SAFE;
CREATE FUNCTION traceprov_get_generic_derivation_spec (BOOLEAN) RETURNS text AS '$libdir/__FILE__',
'traceprov_get_generic_derivation_spec' LANGUAGE C STRICT PARALLEL SAFE;
CREATE FUNCTION traceprov_prepare_for_scan () RETURNS text AS '$libdir/__FILE__',
'traceprov_prepare_for_scan' LANGUAGE C STRICT PARALLEL SAFE;
CREATE FUNCTION traceprov_run_duckdb_query (cstring) RETURNS SETOF record AS '$libdir/__FILE__',
'traceprov_run_duckdb_query' LANGUAGE C STRICT PARALLEL SAFE;
CREATE FUNCTION traceprov_tableam_handler(internal) RETURNS table_am_handler AS '$libdir/__FILE__',
'traceprov_tableam_handler' LANGUAGE C STRICT;
CREATE ACCESS METHOD traceprov_am TYPE TABLE HANDLER traceprov_tableam_handler;
CREATE FUNCTION traceprov_get_infer_stat (BIGINT, BOOLEAN) RETURNS text AS '$libdir/__FILE__',
'traceprov_get_infer_stat' LANGUAGE C STRICT PARALLEL SAFE;
CREATE FUNCTION traceprov_get_total_layer_size () RETURNS text AS '$libdir/__FILE__',
'traceprov_get_total_layer_size' LANGUAGE C STRICT PARALLEL SAFE;