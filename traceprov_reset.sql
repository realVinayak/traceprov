DROP AGGREGATE IF EXISTS traceprov_agg_key (BIGINT);
DROP AGGREGATE IF EXISTS traceprov_agg_key (int, BIGINT);
DROP AGGREGATE IF EXISTS traceprov_agg_key_parallel (int, BIGINT);
DROP AGGREGATE IF EXISTS traceprov_agg_key_parallel (int, BIGINT, BIGINT);
DROP AGGREGATE IF EXISTS traceprov_agg_key_parallel (int, BIGINT, BIGINT, BIGINT);
DROP AGGREGATE IF EXISTS traceprov_agg_key_parallel (int, BIGINT, BIGINT, BIGINT, BIGINT);
DROP AGGREGATE IF EXISTS traceprov_agg_key_parallel (int, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP AGGREGATE IF EXISTS traceprov_agg_key_parallel (
    int,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
);
DROP AGGREGATE IF EXISTS traceprov_agg_key_parallel (
    int,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
);
DROP AGGREGATE IF EXISTS traceprov_agg_key_parallel (
    int,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
);
DROP AGGREGATE IF EXISTS traceprov_agg_key_parallel (
    int,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
);
DROP AGGREGATE IF EXISTS traceprov_agg_key_parallel (
    int,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
);
-- traceprov_agg_key_offset aggregate.
DROP AGGREGATE IF EXISTS traceprov_agg_key_parallel_offset (int, BIGINT);
DROP AGGREGATE IF EXISTS traceprov_agg_key_parallel_offset (int, BIGINT, BIGINT);
DROP AGGREGATE IF EXISTS traceprov_agg_key_parallel_offset (int, BIGINT, BIGINT, BIGINT);
DROP AGGREGATE IF EXISTS traceprov_agg_key_parallel_offset (int, BIGINT, BIGINT, BIGINT, BIGINT);
DROP AGGREGATE IF EXISTS traceprov_agg_key_parallel_offset (int, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP AGGREGATE IF EXISTS traceprov_agg_key_parallel_offset (
    int,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
);
DROP AGGREGATE IF EXISTS traceprov_agg_key_parallel_offset (
    int,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
);
DROP AGGREGATE IF EXISTS traceprov_agg_key_parallel_offset (
    int,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
);
DROP AGGREGATE IF EXISTS traceprov_agg_key_parallel_offset (
    int,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
);
DROP AGGREGATE IF EXISTS traceprov_agg_key_parallel_offset (
    int,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
);
-- traceprov_agg_from_ptr aggregate
DROP AGGREGATE IF EXISTS traceprov_agg_from_ptr (int, int, BIGINT);
DROP AGGREGATE IF EXISTS traceprov_agg_from_ptr_dup_aware (int, int, BIGINT);
-- Functions
DROP FUNCTION IF EXISTS reinit_state (INTEGER);
-- This is here for historical reasons.
DROP FUNCTION IF EXISTS reinit_state ();
DROP FUNCTION IF EXISTS test_local_setup (INTEGER, INTEGER);
DROP FUNCTION IF EXISTS traceprov_agg_key_sfunc (state bigint, bigint);
DROP FUNCTION IF EXISTS traceprov_agg_key_sfunc (state bigint, int, BIGINT);
DROP FUNCTION IF EXISTS traceprov_agg_key_sfunc (state bigint, int, BIGINT, BIGINT);
DROP FUNCTION IF EXISTS traceprov_agg_key_sfunc (state bigint, int, BIGINT, BIGINT, BIGINT);
DROP FUNCTION IF EXISTS traceprov_agg_key_sfunc (
    state bigint,
    int,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
);
DROP FUNCTION IF EXISTS traceprov_agg_key_sfunc (
    state bigint,
    int,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
);
DROP FUNCTION IF EXISTS traceprov_agg_key_sfunc (
    state bigint,
    int,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
);
DROP FUNCTION IF EXISTS traceprov_agg_key_sfunc (
    state bigint,
    int,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
);
DROP FUNCTION IF EXISTS traceprov_agg_key_sfunc (
    state bigint,
    int,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
);
DROP FUNCTION IF EXISTS traceprov_agg_key_sfunc (
    state bigint,
    int,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
);
DROP FUNCTION IF EXISTS traceprov_agg_key_sfunc (
    state bigint,
    int,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
);
DROP FUNCTION IF EXISTS traceprov_agg_key_finalfunc (state bigint);
-- Agg key offset (stores the offset.)
DROP FUNCTION IF EXISTS traceprov_agg_key_offset_finalfunc (state bigint);
DROP FUNCTION IF EXISTS traceprov_agg_from_ptr_sfunc (state bigint, INT, INT, BIGINT);
DROP FUNCTION IF EXISTS traceprov_agg_from_ptr_combine (internal, internal);
DROP FUNCTION IF EXISTS traceprov_agg_from_ptr_serialize (internal);
DROP FUNCTION IF EXISTS traceprov_agg_from_ptr_deserialize (bytea, internal);
DROP FUNCTION IF EXISTS traceprov_agg_from_ptr_finalfunc (state bigint);
-- serialize and deserialize
DROP FUNCTION IF EXISTS traceprov_agg_key_serialize (internal);
DROP FUNCTION IF EXISTS traceprov_agg_key_deserialize (bytea, internal);
-- combine
DROP FUNCTION IF EXISTS traceprov_agg_key_combine (bigint, bigint);
-- nops.
DROP AGGREGATE IF EXISTS traceprov_nop (int, BIGINT);
DROP FUNCTION IF EXISTS traceprov_nop_sfunc (state internal, int, BIGINT);
DROP FUNCTION IF EXISTS traceprov_nop_finalfunc (state internal);
DROP FUNCTION IF EXISTS traceprov_nop_combine (internal, internal);
DROP FUNCTION IF EXISTS traceprov_nop_serialize (internal);
DROP FUNCTION IF EXISTS traceprov_nop_deserialize (bytea, internal);
-- log input->pointers
DROP FUNCTION IF EXISTS traceprov_make_ptr (INT, BIGINT);
DROP FUNCTION IF EXISTS traceprov_make_ptr (INT, BIGINT, BIGINT);
DROP FUNCTION IF EXISTS traceprov_make_ptr (INT, BIGINT, BIGINT, BIGINT);
DROP FUNCTION IF EXISTS traceprov_make_ptr (INT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP FUNCTION IF EXISTS traceprov_make_ptr (INT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP FUNCTION IF EXISTS traceprov_make_ptr (
    INT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
);
DROP FUNCTION IF EXISTS traceprov_log_entry (INT, BIGINT);
DROP FUNCTION IF EXISTS traceprov_log_entry (INT, BIGINT, BIGINT);
DROP FUNCTION IF EXISTS traceprov_log_entry (INT, BIGINT, BIGINT, BIGINT);
DROP FUNCTION IF EXISTS traceprov_log_entry (INT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP FUNCTION IF EXISTS traceprov_log_entry (INT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP FUNCTION IF EXISTS traceprov_log_entry (
    INT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
);
DROP FUNCTION IF EXISTS traceprov_log_entry (
    INT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
);
DROP FUNCTION IF EXISTS traceprov_log_entry (
    INT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
);
DROP FUNCTION IF EXISTS traceprov_log_entry (
    INT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
);
DROP FUNCTION IF EXISTS traceprov_log_entry_n (INT, BIGINT, BIGINT);
DROP FUNCTION IF EXISTS traceprov_log_entry_n (INT, BIGINT, BIGINT, BIGINT);
DROP FUNCTION IF EXISTS traceprov_log_entry_n (INT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP FUNCTION IF EXISTS traceprov_log_entry_n (INT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP FUNCTION IF EXISTS traceprov_log_entry_n (
    INT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
);
DROP FUNCTION IF EXISTS traceprov_log_entry_n (
    INT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
);
DROP FUNCTION IF EXISTS traceprov_log_entry_volatile (INT, BIGINT);
DROP FUNCTION IF EXISTS traceprov_log_entry_volatile (INT, BIGINT, BIGINT);
DROP FUNCTION IF EXISTS traceprov_log_entry_volatile (INT, BIGINT, BIGINT, BIGINT);
DROP FUNCTION IF EXISTS traceprov_log_entry_volatile (INT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP FUNCTION IF EXISTS traceprov_log_entry_volatile (INT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP FUNCTION IF EXISTS traceprov_log_entry_volatile (
    INT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
);

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
