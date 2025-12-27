-- traceprov_agg_key aggregate.
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

DROP FUNCTION IF EXISTS mark_later (bigint);

DROP FUNCTION IF EXISTS mark_later_value (bigint);

DROP FUNCTION IF EXISTS test_local_setup (INTEGER, INTEGER);

DROP FUNCTION IF EXISTS traceprov_agg_key_sfunc (state internal, bigint);

DROP FUNCTION IF EXISTS traceprov_agg_key_sfunc (state internal, int, BIGINT);

DROP FUNCTION IF EXISTS traceprov_agg_key_sfunc (state internal, int, BIGINT, BIGINT);

DROP FUNCTION IF EXISTS traceprov_agg_key_sfunc (state internal, int, BIGINT, BIGINT, BIGINT);

DROP FUNCTION IF EXISTS traceprov_agg_key_sfunc (
    state internal,
    int,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
);

DROP FUNCTION IF EXISTS traceprov_agg_key_sfunc (
    state internal,
    int,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
);

DROP FUNCTION IF EXISTS traceprov_agg_key_sfunc (
    state internal,
    int,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
);

DROP FUNCTION IF EXISTS traceprov_agg_key_sfunc (
    state internal,
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
    state internal,
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
    state internal,
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
    state internal,
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

DROP FUNCTION IF EXISTS traceprov_agg_key_finalfunc (state internal);

-- Agg key offset (stores the offset.)
DROP FUNCTION IF EXISTS traceprov_agg_key_offset_finalfunc (state internal);

DROP FUNCTION IF EXISTS traceprov_agg_from_ptr_sfunc (state internal, INT, INT, BIGINT);

DROP FUNCTION IF EXISTS traceprov_agg_from_ptr_combine (internal, internal);

DROP FUNCTION IF EXISTS traceprov_agg_from_ptr_serialize (internal);

DROP FUNCTION IF EXISTS traceprov_agg_from_ptr_deserialize (bytea, internal);

DROP FUNCTION IF EXISTS traceprov_agg_from_ptr_finalfunc (state internal);

-- serialize and deserialize
DROP FUNCTION IF EXISTS traceprov_agg_key_serialize (internal);

DROP FUNCTION IF EXISTS traceprov_agg_key_deserialize (bytea, internal);

-- combine
DROP FUNCTION IF EXISTS traceprov_agg_key_combine (internal, internal);

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

CREATE FUNCTION reinit_state () RETURNS INTEGER AS '$libdir/__FILE__',
'reinit_state' LANGUAGE C;

CREATE FUNCTION test_local_setup (INTEGER, INTEGER) RETURNS INTEGER AS '$libdir/__FILE__',
'test_local_setup' LANGUAGE C;

CREATE FUNCTION traceprov_agg_key_sfunc (state internal, int, bigint) RETURNS internal AS '$libdir/__FILE__',
'traceprov_agg_key_sfunc' LANGUAGE C;

CREATE FUNCTION traceprov_agg_key_finalfunc (state internal) RETURNS bigint AS '$libdir/__FILE__',
'traceprov_agg_key_finalfunc' LANGUAGE C;

CREATE FUNCTION traceprov_agg_key_serialize (internal) RETURNS bytea AS '$libdir/__FILE__',
'traceprov_agg_key_serialize' LANGUAGE C;

CREATE FUNCTION traceprov_agg_key_deserialize (bytea, internal) RETURNS internal AS '$libdir/__FILE__',
'traceprov_agg_key_deserialize' LANGUAGE C;

CREATE FUNCTION traceprov_agg_key_combine (internal, internal) RETURNS internal AS '$libdir/__FILE__',
'traceprov_agg_key_combine' LANGUAGE C;

CREATE FUNCTION mark_later (bigint) RETURNS INTEGER AS '$libdir/__FILE__',
'mark_later' LANGUAGE C PARALLEL SAFE;

CREATE FUNCTION mark_later_value (bigint, bigint) RETURNS INTEGER AS '$libdir/__FILE__',
'mark_later_value' LANGUAGE C PARALLEL SAFE;

CREATE FUNCTION traceprov_agg_key_offset_finalfunc (state internal) RETURNS bigint AS '$libdir/__FILE__',
'traceprov_agg_key_offset_finalfunc' LANGUAGE C;

CREATE AGGREGATE traceprov_agg_key (int, BIGINT) (
    SFUNC = traceprov_agg_key_sfunc,
    STYPE = internal,
    FINALFUNC = traceprov_agg_key_finalfunc,
    SSPACE = 32
);

CREATE FUNCTION traceprov_agg_key_sfunc (state internal, int, BIGINT, BIGINT) RETURNS internal AS '$libdir/__FILE__',
'traceprov_agg_key_sfunc' LANGUAGE C;

CREATE FUNCTION traceprov_agg_key_sfunc (state internal, int, BIGINT, BIGINT, BIGINT) RETURNS internal AS '$libdir/__FILE__',
'traceprov_agg_key_sfunc' LANGUAGE C;

CREATE FUNCTION traceprov_agg_key_sfunc (
    state internal,
    int,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
) RETURNS internal AS '$libdir/__FILE__',
'traceprov_agg_key_sfunc' LANGUAGE C;

CREATE FUNCTION traceprov_agg_key_sfunc (
    state internal,
    int,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
) RETURNS internal AS '$libdir/__FILE__',
'traceprov_agg_key_sfunc' LANGUAGE C;

CREATE FUNCTION traceprov_agg_key_sfunc (
    state internal,
    int,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
) RETURNS internal AS '$libdir/__FILE__',
'traceprov_agg_key_sfunc' LANGUAGE C;

CREATE FUNCTION traceprov_agg_key_sfunc (
    state internal,
    int,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
) RETURNS internal AS '$libdir/__FILE__',
'traceprov_agg_key_sfunc' LANGUAGE C;

CREATE FUNCTION traceprov_agg_key_sfunc (
    state internal,
    int,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
) RETURNS internal AS '$libdir/__FILE__',
'traceprov_agg_key_sfunc' LANGUAGE C;

CREATE FUNCTION traceprov_agg_key_sfunc (
    state internal,
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
) RETURNS internal AS '$libdir/__FILE__',
'traceprov_agg_key_sfunc' LANGUAGE C;

CREATE FUNCTION traceprov_agg_key_sfunc (
    state internal,
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
) RETURNS internal AS '$libdir/__FILE__',
'traceprov_agg_key_sfunc' LANGUAGE C;

CREATE FUNCTION traceprov_nop_sfunc (state internal, int, BIGINT) RETURNS internal AS '$libdir/__FILE__',
'traceprov_nop_sfunc' LANGUAGE C;

CREATE FUNCTION traceprov_nop_finalfunc (state internal) RETURNS bigint AS '$libdir/__FILE__',
'traceprov_nop_finalfunc' LANGUAGE C;

CREATE FUNCTION traceprov_nop_combine (internal, internal) RETURNS internal AS '$libdir/__FILE__',
'traceprov_nop_combine' LANGUAGE C;

CREATE FUNCTION traceprov_nop_serialize (internal) RETURNS bytea AS '$libdir/__FILE__',
'traceprov_nop_serialize' LANGUAGE C;

CREATE FUNCTION traceprov_nop_deserialize (bytea, internal) RETURNS internal AS '$libdir/__FILE__',
'traceprov_nop_deserialize' LANGUAGE C;

CREATE AGGREGATE traceprov_nop (int, BIGINT) (
    SFUNC = traceprov_nop_sfunc,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = traceprov_nop_finalfunc,
    COMBINEFUNC = traceprov_nop_combine,
    SERIALFUNC = traceprov_nop_serialize,
    DESERIALFUNC = traceprov_nop_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE traceprov_agg_key_parallel (int, BIGINT) (
    SFUNC = traceprov_agg_key_sfunc,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = traceprov_agg_key_finalfunc,
    COMBINEFUNC = traceprov_agg_key_combine,
    SERIALFUNC = traceprov_agg_key_serialize,
    DESERIALFUNC = traceprov_agg_key_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE traceprov_agg_key_parallel (int, BIGINT, BIGINT) (
    SFUNC = traceprov_agg_key_sfunc,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = traceprov_agg_key_finalfunc,
    COMBINEFUNC = traceprov_agg_key_combine,
    SERIALFUNC = traceprov_agg_key_serialize,
    DESERIALFUNC = traceprov_agg_key_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE traceprov_agg_key_parallel (int, BIGINT, BIGINT, BIGINT) (
    SFUNC = traceprov_agg_key_sfunc,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = traceprov_agg_key_finalfunc,
    COMBINEFUNC = traceprov_agg_key_combine,
    SERIALFUNC = traceprov_agg_key_serialize,
    DESERIALFUNC = traceprov_agg_key_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE traceprov_agg_key_parallel (int, BIGINT, BIGINT, BIGINT, BIGINT) (
    SFUNC = traceprov_agg_key_sfunc,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = traceprov_agg_key_finalfunc,
    COMBINEFUNC = traceprov_agg_key_combine,
    SERIALFUNC = traceprov_agg_key_serialize,
    DESERIALFUNC = traceprov_agg_key_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE traceprov_agg_key_parallel (int, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) (
    SFUNC = traceprov_agg_key_sfunc,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = traceprov_agg_key_finalfunc,
    COMBINEFUNC = traceprov_agg_key_combine,
    SERIALFUNC = traceprov_agg_key_serialize,
    DESERIALFUNC = traceprov_agg_key_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE traceprov_agg_key_parallel (
    int,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
) (
    SFUNC = traceprov_agg_key_sfunc,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = traceprov_agg_key_finalfunc,
    COMBINEFUNC = traceprov_agg_key_combine,
    SERIALFUNC = traceprov_agg_key_serialize,
    DESERIALFUNC = traceprov_agg_key_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE traceprov_agg_key_parallel (
    int,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
) (
    SFUNC = traceprov_agg_key_sfunc,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = traceprov_agg_key_finalfunc,
    COMBINEFUNC = traceprov_agg_key_combine,
    SERIALFUNC = traceprov_agg_key_serialize,
    DESERIALFUNC = traceprov_agg_key_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE traceprov_agg_key_parallel (
    int,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
) (
    SFUNC = traceprov_agg_key_sfunc,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = traceprov_agg_key_finalfunc,
    COMBINEFUNC = traceprov_agg_key_combine,
    SERIALFUNC = traceprov_agg_key_serialize,
    DESERIALFUNC = traceprov_agg_key_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE traceprov_agg_key_parallel (
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
) (
    SFUNC = traceprov_agg_key_sfunc,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = traceprov_agg_key_finalfunc,
    COMBINEFUNC = traceprov_agg_key_combine,
    SERIALFUNC = traceprov_agg_key_serialize,
    DESERIALFUNC = traceprov_agg_key_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE traceprov_agg_key_parallel (
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
) (
    SFUNC = traceprov_agg_key_sfunc,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = traceprov_agg_key_finalfunc,
    COMBINEFUNC = traceprov_agg_key_combine,
    SERIALFUNC = traceprov_agg_key_serialize,
    DESERIALFUNC = traceprov_agg_key_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE traceprov_agg_key_parallel_offset (int, BIGINT) (
    SFUNC = traceprov_agg_key_sfunc,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = traceprov_agg_key_offset_finalfunc,
    COMBINEFUNC = traceprov_agg_key_combine,
    SERIALFUNC = traceprov_agg_key_serialize,
    DESERIALFUNC = traceprov_agg_key_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE traceprov_agg_key_parallel_offset (int, BIGINT, BIGINT) (
    SFUNC = traceprov_agg_key_sfunc,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = traceprov_agg_key_offset_finalfunc,
    COMBINEFUNC = traceprov_agg_key_combine,
    SERIALFUNC = traceprov_agg_key_serialize,
    DESERIALFUNC = traceprov_agg_key_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE traceprov_agg_key_parallel_offset (int, BIGINT, BIGINT, BIGINT) (
    SFUNC = traceprov_agg_key_sfunc,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = traceprov_agg_key_offset_finalfunc,
    COMBINEFUNC = traceprov_agg_key_combine,
    SERIALFUNC = traceprov_agg_key_serialize,
    DESERIALFUNC = traceprov_agg_key_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE traceprov_agg_key_parallel_offset (int, BIGINT, BIGINT, BIGINT, BIGINT) (
    SFUNC = traceprov_agg_key_sfunc,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = traceprov_agg_key_offset_finalfunc,
    COMBINEFUNC = traceprov_agg_key_combine,
    SERIALFUNC = traceprov_agg_key_serialize,
    DESERIALFUNC = traceprov_agg_key_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE traceprov_agg_key_parallel_offset (int, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) (
    SFUNC = traceprov_agg_key_sfunc,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = traceprov_agg_key_offset_finalfunc,
    COMBINEFUNC = traceprov_agg_key_combine,
    SERIALFUNC = traceprov_agg_key_serialize,
    DESERIALFUNC = traceprov_agg_key_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE traceprov_agg_key_parallel_offset (
    int,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
) (
    SFUNC = traceprov_agg_key_sfunc,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = traceprov_agg_key_offset_finalfunc,
    COMBINEFUNC = traceprov_agg_key_combine,
    SERIALFUNC = traceprov_agg_key_serialize,
    DESERIALFUNC = traceprov_agg_key_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE traceprov_agg_key_parallel_offset (
    int,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
) (
    SFUNC = traceprov_agg_key_sfunc,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = traceprov_agg_key_offset_finalfunc,
    COMBINEFUNC = traceprov_agg_key_combine,
    SERIALFUNC = traceprov_agg_key_serialize,
    DESERIALFUNC = traceprov_agg_key_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE traceprov_agg_key_parallel_offset (
    int,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
) (
    SFUNC = traceprov_agg_key_sfunc,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = traceprov_agg_key_offset_finalfunc,
    COMBINEFUNC = traceprov_agg_key_combine,
    SERIALFUNC = traceprov_agg_key_serialize,
    DESERIALFUNC = traceprov_agg_key_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE traceprov_agg_key_parallel_offset (
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
) (
    SFUNC = traceprov_agg_key_sfunc,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = traceprov_agg_key_offset_finalfunc,
    COMBINEFUNC = traceprov_agg_key_combine,
    SERIALFUNC = traceprov_agg_key_serialize,
    DESERIALFUNC = traceprov_agg_key_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE traceprov_agg_key_parallel_offset (
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
) (
    SFUNC = traceprov_agg_key_sfunc,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = traceprov_agg_key_offset_finalfunc,
    COMBINEFUNC = traceprov_agg_key_combine,
    SERIALFUNC = traceprov_agg_key_serialize,
    DESERIALFUNC = traceprov_agg_key_deserialize,
    PARALLEL = SAFE
);

CREATE FUNCTION traceprov_agg_from_ptr_sfunc (state internal, int, int, BIGINT) RETURNS internal AS '$libdir/__FILE__',
'traceprov_agg_from_ptr_sfunc' LANGUAGE C;

CREATE FUNCTION traceprov_agg_from_ptr_finalfunc (state internal) RETURNS bigint AS '$libdir/__FILE__',
'traceprov_agg_from_ptr_finalfunc' LANGUAGE C;

CREATE FUNCTION traceprov_agg_from_ptr_serialize (internal) RETURNS bytea AS '$libdir/__FILE__',
'traceprov_agg_from_ptr_serialize' LANGUAGE C;

CREATE FUNCTION traceprov_agg_from_ptr_deserialize (bytea, internal) RETURNS internal AS '$libdir/__FILE__',
'traceprov_agg_from_ptr_deserialize' LANGUAGE C;

CREATE FUNCTION traceprov_agg_from_ptr_combine (internal, internal) RETURNS internal AS '$libdir/__FILE__',
'traceprov_agg_from_ptr_combine' LANGUAGE C;

CREATE AGGREGATE traceprov_agg_from_ptr (int, int, BIGINT) (
    SFUNC = traceprov_agg_from_ptr_sfunc,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = traceprov_agg_from_ptr_finalfunc,
    COMBINEFUNC = traceprov_agg_from_ptr_combine,
    SERIALFUNC = traceprov_agg_from_ptr_serialize,
    DESERIALFUNC = traceprov_agg_from_ptr_deserialize,
    PARALLEL = SAFE
);

CREATE FUNCTION traceprov_make_ptr (INTEGER, BIGINT) RETURNS BIGINT AS '$libdir/__FILE__',
'traceprov_make_ptr' LANGUAGE C PARALLEL SAFE STABLE;

CREATE FUNCTION traceprov_make_ptr (INTEGER, BIGINT, BIGINT) RETURNS BIGINT AS '$libdir/__FILE__',
'traceprov_make_ptr' LANGUAGE C PARALLEL SAFE STABLE;

CREATE FUNCTION traceprov_make_ptr (INTEGER, BIGINT, BIGINT, BIGINT) RETURNS BIGINT AS '$libdir/__FILE__',
'traceprov_make_ptr' LANGUAGE C PARALLEL SAFE STABLE;

CREATE FUNCTION traceprov_make_ptr (INTEGER, BIGINT, BIGINT, BIGINT, BIGINT) RETURNS BIGINT AS '$libdir/__FILE__',
'traceprov_make_ptr' LANGUAGE C PARALLEL SAFE STABLE;

CREATE FUNCTION traceprov_make_ptr (INTEGER, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) RETURNS BIGINT AS '$libdir/__FILE__',
'traceprov_make_ptr' LANGUAGE C PARALLEL SAFE STABLE;

CREATE FUNCTION traceprov_make_ptr (
    INTEGER,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
) RETURNS BIGINT AS '$libdir/__FILE__',
'traceprov_make_ptr' LANGUAGE C PARALLEL SAFE STABLE;

CREATE FUNCTION traceprov_log_entry (INTEGER, BIGINT) RETURNS BOOLEAN AS '$libdir/__FILE__',
'traceprov_log_entry' LANGUAGE C PARALLEL SAFE STABLE;

CREATE FUNCTION traceprov_log_entry (INTEGER, BIGINT, BIGINT) RETURNS BOOLEAN AS '$libdir/__FILE__',
'traceprov_log_entry' LANGUAGE C PARALLEL SAFE STABLE;

CREATE FUNCTION traceprov_log_entry (INTEGER, BIGINT, BIGINT, BIGINT) RETURNS BOOLEAN AS '$libdir/__FILE__',
'traceprov_log_entry' LANGUAGE C PARALLEL SAFE STABLE;

CREATE FUNCTION traceprov_log_entry (INTEGER, BIGINT, BIGINT, BIGINT, BIGINT) RETURNS BOOLEAN AS '$libdir/__FILE__',
'traceprov_log_entry' LANGUAGE C PARALLEL SAFE STABLE;

CREATE FUNCTION traceprov_log_entry (INTEGER, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) RETURNS BOOLEAN AS '$libdir/__FILE__',
'traceprov_log_entry' LANGUAGE C PARALLEL SAFE STABLE;

CREATE FUNCTION traceprov_log_entry (
    INTEGER,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
) RETURNS BOOLEAN AS '$libdir/__FILE__',
'traceprov_log_entry' LANGUAGE C PARALLEL SAFE STABLE;

CREATE FUNCTION traceprov_log_entry_n (INTEGER, BIGINT, BIGINT) RETURNS BIGINT AS '$libdir/__FILE__',
'traceprov_log_entry_n' LANGUAGE C PARALLEL SAFE STABLE;

CREATE FUNCTION traceprov_log_entry_n (INTEGER, BIGINT, BIGINT, BIGINT) RETURNS BIGINT AS '$libdir/__FILE__',
'traceprov_log_entry_n' LANGUAGE C PARALLEL SAFE STABLE;

CREATE FUNCTION traceprov_log_entry_n (INTEGER, BIGINT, BIGINT, BIGINT, BIGINT) RETURNS BIGINT AS '$libdir/__FILE__',
'traceprov_log_entry_n' LANGUAGE C PARALLEL SAFE STABLE;

CREATE FUNCTION traceprov_log_entry_n (INTEGER, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) RETURNS BIGINT AS '$libdir/__FILE__',
'traceprov_log_entry_n' LANGUAGE C PARALLEL SAFE STABLE;

CREATE FUNCTION traceprov_log_entry_n (
    INTEGER,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
) RETURNS BIGINT AS '$libdir/__FILE__',
'traceprov_log_entry_n' LANGUAGE C PARALLEL SAFE STABLE;

CREATE FUNCTION traceprov_log_entry_n (
    INTEGER,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
) RETURNS BIGINT AS '$libdir/__FILE__',
'traceprov_log_entry_n' LANGUAGE C PARALLEL SAFE STABLE;

CREATE FUNCTION traceprov_log_entry_volatile (INTEGER, BIGINT) RETURNS BOOLEAN AS '$libdir/__FILE__',
'traceprov_log_entry' LANGUAGE C PARALLEL SAFE VOLATILE;

CREATE FUNCTION traceprov_log_entry_volatile (INTEGER, BIGINT, BIGINT) RETURNS BOOLEAN AS '$libdir/__FILE__',
'traceprov_log_entry' LANGUAGE C PARALLEL SAFE VOLATILE;

CREATE FUNCTION traceprov_log_entry_volatile (INTEGER, BIGINT, BIGINT, BIGINT) RETURNS BOOLEAN AS '$libdir/__FILE__',
'traceprov_log_entry' LANGUAGE C PARALLEL SAFE VOLATILE;

CREATE FUNCTION traceprov_log_entry_volatile (INTEGER, BIGINT, BIGINT, BIGINT, BIGINT) RETURNS BOOLEAN AS '$libdir/__FILE__',
'traceprov_log_entry' LANGUAGE C PARALLEL SAFE VOLATILE;

CREATE FUNCTION traceprov_log_entry_volatile (INTEGER, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) RETURNS BOOLEAN AS '$libdir/__FILE__',
'traceprov_log_entry' LANGUAGE C PARALLEL SAFE VOLATILE;

CREATE FUNCTION traceprov_log_entry_volatile (
    INTEGER,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT,
    BIGINT
) RETURNS BOOLEAN AS '$libdir/__FILE__',
'traceprov_log_entry' LANGUAGE C PARALLEL SAFE VOLATILE;

DROP TYPE IF EXISTS traceprov_ptr_type CASCADE;

CREATE FUNCTION traceprov_ptr_type_in (cstring) RETURNS traceprov_ptr_type AS '$libdir/__FILE__',
'traceprov_ptr_type_in' LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION traceprov_ptr_type_out (traceprov_ptr_type) RETURNS cstring AS '$libdir/__FILE__',
'traceprov_ptr_type_out' LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION traceprov_ptr_type_recv (internal) RETURNS traceprov_ptr_type AS '$libdir/__FILE__',
'traceprov_ptr_type_recv' LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION traceprov_ptr_type_send (traceprov_ptr_type) RETURNS bytea AS '$libdir/__FILE__',
'traceprov_ptr_type_send' LANGUAGE C IMMUTABLE STRICT;

CREATE TYPE traceprov_ptr_type (
    internallength = 8,
    input = traceprov_ptr_type_in,
    output = traceprov_ptr_type_out,
    receive = traceprov_ptr_type_recv,
    send = traceprov_ptr_type_send,
    alignment = double,
    PASSEDBYVALUE
);

-- Sort operators.
CREATE FUNCTION traceprov_ptr_type_lt (traceprov_ptr_type, traceprov_ptr_type) RETURNS bool AS '$libdir/__FILE__',
'traceprov_ptr_type_lt' LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION traceprov_ptr_type_gt (traceprov_ptr_type, traceprov_ptr_type) RETURNS bool AS '$libdir/__FILE__',
'traceprov_ptr_type_gt' LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION traceprov_ptr_type_le (traceprov_ptr_type, traceprov_ptr_type) RETURNS bool AS '$libdir/__FILE__',
'traceprov_ptr_type_le' LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION traceprov_ptr_type_eq (traceprov_ptr_type, traceprov_ptr_type) RETURNS bool AS '$libdir/__FILE__',
'traceprov_ptr_type_eq' LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION traceprov_ptr_type_ge (traceprov_ptr_type, traceprov_ptr_type) RETURNS bool AS '$libdir/__FILE__',
'traceprov_ptr_type_ge' LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR < (
    leftarg = traceprov_ptr_type,
    rightarg = traceprov_ptr_type,
    procedure = traceprov_ptr_type_lt,
    commutator = >,
    negator = >=,
    restrict = scalarltsel,
    join = scalarltjoinsel
);

CREATE OPERATOR <= (
    leftarg = traceprov_ptr_type,
    rightarg = traceprov_ptr_type,
    procedure = traceprov_ptr_type_le,
    commutator = >=,
    negator = >,
    restrict = scalarlesel,
    join = scalarlejoinsel
);

CREATE OPERATOR = (
    leftarg = traceprov_ptr_type,
    rightarg = traceprov_ptr_type,
    procedure = traceprov_ptr_type_eq,
    commutator = =,
    -- leave out negator since we didn't create <> operator
    -- negator = <> ,
    restrict = eqsel,
    join = eqjoinsel
);

CREATE OPERATOR >= (
    leftarg = traceprov_ptr_type,
    rightarg = traceprov_ptr_type,
    procedure = traceprov_ptr_type_ge,
    commutator = <=,
    negator = <,
    restrict = scalargesel,
    join = scalargejoinsel
);

CREATE OPERATOR > (
    leftarg = traceprov_ptr_type,
    rightarg = traceprov_ptr_type,
    procedure = traceprov_ptr_type_gt,
    commutator = <,
    negator = <=,
    restrict = scalargtsel,
    join = scalargtjoinsel
);

CREATE FUNCTION traceprov_ptr_type_abs_cmp (traceprov_ptr_type, traceprov_ptr_type) RETURNS int4 AS '$libdir/__FILE__',
'traceprov_ptr_type_cmp' LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR CLASS traceprov_ptr_type_abs_ops DEFAULT FOR
TYPE traceprov_ptr_type USING btree AS OPERATOR 1 <,
OPERATOR 2 <=,
OPERATOR 3 =,
OPERATOR 4 >=,
OPERATOR 5 >,
FUNCTION 1 traceprov_ptr_type_abs_cmp (traceprov_ptr_type, traceprov_ptr_type);

-- Both of the below don't cause a function invocation.
create cast (traceprov_ptr_type as bigint) without function as implicit;

create cast (bigint as traceprov_ptr_type) without function as implicit;