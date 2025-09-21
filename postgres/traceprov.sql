-- traceprov_agg_key aggregate.
DROP AGGREGATE traceprov_agg_key(BIGINT);
DROP AGGREGATE traceprov_agg_key(int, BIGINT);
DROP AGGREGATE traceprov_agg_key_parallel(int, BIGINT);
DROP AGGREGATE traceprov_agg_key_parallel(int, BIGINT, BIGINT);
DROP AGGREGATE traceprov_agg_key_parallel(int, BIGINT, BIGINT, BIGINT);
DROP AGGREGATE traceprov_agg_key_parallel(int, BIGINT, BIGINT, BIGINT, BIGINT);
DROP AGGREGATE traceprov_agg_key_parallel(int, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP AGGREGATE traceprov_agg_key_parallel(int, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP AGGREGATE traceprov_agg_key_parallel(int, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP AGGREGATE traceprov_agg_key_parallel(int, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP AGGREGATE traceprov_agg_key_parallel(int, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP AGGREGATE traceprov_agg_key_parallel(int, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);

-- traceprov_agg_from_ptr aggregate
DROP AGGREGATE traceprov_agg_from_ptr(int, int, BIGINT);
DROP AGGREGATE traceprov_agg_from_ptr_dup_aware(int, int, BIGINT);

-- Functions
DROP FUNCTION   IF EXISTS reinit_state(INTEGER); -- This is here for historical reasons.
DROP FUNCTION   IF EXISTS reinit_state();
DROP FUNCTION   IF EXISTS mark_later(bigint);
DROP FUNCTION   IF EXISTS test_local_setup(INTEGER, INTEGER);
DROP FUNCTION   IF EXISTS traceprov_agg_key_sfunc(state internal, bigint);
DROP FUNCTION   IF EXISTS traceprov_agg_key_sfunc(state internal, int, BIGINT);
DROP FUNCTION   IF EXISTS traceprov_agg_key_sfunc(state internal, int, BIGINT, BIGINT);
DROP FUNCTION   IF EXISTS traceprov_agg_key_sfunc(state internal, int, BIGINT, BIGINT, BIGINT);
DROP FUNCTION   IF EXISTS traceprov_agg_key_sfunc(state internal, int, BIGINT, BIGINT, BIGINT, BIGINT);
DROP FUNCTION   IF EXISTS traceprov_agg_key_sfunc(state internal, int, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP FUNCTION   IF EXISTS traceprov_agg_key_sfunc(state internal, int, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP FUNCTION   IF EXISTS traceprov_agg_key_sfunc(state internal, int, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP FUNCTION   IF EXISTS traceprov_agg_key_sfunc(state internal, int, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP FUNCTION   IF EXISTS traceprov_agg_key_sfunc(state internal, int, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP FUNCTION   IF EXISTS traceprov_agg_key_sfunc(state internal, int, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP FUNCTION   IF EXISTS traceprov_agg_key_finalfunc(state internal);

DROP FUNCTION   IF EXISTS traceprov_agg_from_ptr_sfunc(state internal, INT, INT, BIGINT);
DROP FUNCTION   IF EXISTS traceprov_agg_from_ptr_combine(internal, internal);
DROP FUNCTION   IF EXISTS traceprov_agg_from_ptr_serialize(internal);
DROP FUNCTION   IF EXISTS traceprov_agg_from_ptr_deserialize(bytea, internal);
DROP FUNCTION   IF EXISTS traceprov_agg_from_ptr_finalfunc(state internal);

-- serialize and deserialize
DROP FUNCTION   IF EXISTS traceprov_agg_key_serialize(internal);
DROP FUNCTION   IF EXISTS traceprov_agg_key_deserialize(bytea, internal);

-- combine
DROP FUNCTION   IF EXISTS traceprov_agg_key_combine(internal, internal);

-- log subqquery pk.
DROP FUNCTION   IF EXISTS traceprov_log_subquery_pk(INT, BIGINT);
DROP FUNCTION   IF EXISTS traceprov_log_subquery_pk(INT, BIGINT, BIGINT);
DROP FUNCTION   IF EXISTS traceprov_log_subquery_pk(INT, BIGINT, BIGINT, BIGINT);
DROP FUNCTION   IF EXISTS traceprov_log_subquery_pk(INT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP FUNCTION   IF EXISTS traceprov_log_subquery_pk(INT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP FUNCTION   IF EXISTS traceprov_log_subquery_pk(INT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);

CREATE FUNCTION reinit_state() RETURNS INTEGER AS '$libdir/__FILE__', 'reinit_state' LANGUAGE C;

CREATE FUNCTION test_local_setup(INTEGER, INTEGER) RETURNS INTEGER AS '$libdir/__FILE__', 'test_local_setup' LANGUAGE C;

CREATE FUNCTION traceprov_agg_key_sfunc(state internal, int, bigint) RETURNS internal AS '$libdir/__FILE__', 'traceprov_agg_key_sfunc' LANGUAGE C;
CREATE FUNCTION traceprov_agg_key_finalfunc(state internal) RETURNS bigint AS '$libdir/__FILE__', 'traceprov_agg_key_finalfunc' LANGUAGE C;
CREATE FUNCTION traceprov_agg_key_serialize(internal) RETURNS bytea AS '$libdir/__FILE__', 'traceprov_agg_key_serialize' LANGUAGE C;
CREATE FUNCTION traceprov_agg_key_deserialize(bytea, internal) RETURNS internal AS '$libdir/__FILE__', 'traceprov_agg_key_deserialize' LANGUAGE C;
CREATE FUNCTION traceprov_agg_key_combine(internal, internal) RETURNS internal AS '$libdir/__FILE__', 'traceprov_agg_key_combine' LANGUAGE C;

CREATE FUNCTION mark_later(bigint) RETURNS INTEGER AS '$libdir/__FILE__', 'mark_later' LANGUAGE C PARALLEL SAFE;

CREATE AGGREGATE traceprov_agg_key(int, BIGINT) (
    SFUNC = traceprov_agg_key_sfunc, 
    STYPE = internal, 
    FINALFUNC = traceprov_agg_key_finalfunc, 
    SSPACE = 32
);

CREATE FUNCTION traceprov_agg_key_sfunc(state internal, int, BIGINT, BIGINT) RETURNS internal AS '$libdir/__FILE__', 'traceprov_agg_key_sfunc' LANGUAGE C;
CREATE FUNCTION traceprov_agg_key_sfunc(state internal, int, BIGINT, BIGINT, BIGINT) RETURNS internal AS '$libdir/__FILE__', 'traceprov_agg_key_sfunc' LANGUAGE C;
CREATE FUNCTION traceprov_agg_key_sfunc(state internal, int, BIGINT, BIGINT, BIGINT, BIGINT) RETURNS internal AS '$libdir/__FILE__', 'traceprov_agg_key_sfunc' LANGUAGE C;
CREATE FUNCTION traceprov_agg_key_sfunc(state internal, int, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) RETURNS internal AS '$libdir/__FILE__', 'traceprov_agg_key_sfunc' LANGUAGE C;
CREATE FUNCTION traceprov_agg_key_sfunc(state internal, int, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) RETURNS internal AS '$libdir/__FILE__', 'traceprov_agg_key_sfunc' LANGUAGE C;
CREATE FUNCTION traceprov_agg_key_sfunc(state internal, int, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) RETURNS internal AS '$libdir/__FILE__', 'traceprov_agg_key_sfunc' LANGUAGE C;
CREATE FUNCTION traceprov_agg_key_sfunc(state internal, int, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) RETURNS internal AS '$libdir/__FILE__', 'traceprov_agg_key_sfunc' LANGUAGE C;
CREATE FUNCTION traceprov_agg_key_sfunc(state internal, int, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) RETURNS internal AS '$libdir/__FILE__', 'traceprov_agg_key_sfunc' LANGUAGE C;
CREATE FUNCTION traceprov_agg_key_sfunc(state internal, int, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) RETURNS internal AS '$libdir/__FILE__', 'traceprov_agg_key_sfunc' LANGUAGE C;

CREATE FUNCTION traceprov_nop_sfunc(state internal, int, BIGINT) RETURNS internal AS '$libdir/__FILE__', 'traceprov_nop_sfunc' LANGUAGE C;
CREATE FUNCTION traceprov_nop_finalfunc(state internal) RETURNS bigint AS '$libdir/__FILE__', 'traceprov_nop_finalfunc' LANGUAGE C;
CREATE FUNCTION traceprov_nop_combine(state internal, state internal) RETURNS internal AS '$libdir/__FILE__', 'traceprov_nop_combine' LANGUAGE C;
CREATE FUNCTION traceprov_nop_serialize(internal) RETURNS bytea AS '$libdir/__FILE__', 'traceprov_nop_serialize' LANGUAGE C;
CREATE FUNCTION traceprov_nop_deserialize(bytea, internal) RETURNS internal AS '$libdir/__FILE__', 'traceprov_nop_deserialize' LANGUAGE C;

CREATE AGGREGATE traceprov_nop(int, BIGINT) (
    SFUNC = traceprov_nop_sfunc, 
    STYPE = internal, 
    SSPACE = 32,
    FINALFUNC = traceprov_nop_finalfunc, 
    COMBINEFUNC = traceprov_nop_combine,
    SERIALFUNC = traceprov_nop_serialize,
    DESERIALFUNC = traceprov_nop_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE traceprov_agg_key_parallel(int, BIGINT) (
    SFUNC = traceprov_agg_key_sfunc, 
    STYPE = internal, 
    SSPACE = 32,
    FINALFUNC = traceprov_agg_key_finalfunc, 
    COMBINEFUNC = traceprov_agg_key_combine,
    SERIALFUNC = traceprov_agg_key_serialize,
    DESERIALFUNC = traceprov_agg_key_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE traceprov_agg_key_parallel(int, BIGINT, BIGINT) (
    SFUNC = traceprov_agg_key_sfunc, 
    STYPE = internal, 
    SSPACE = 32,
    FINALFUNC = traceprov_agg_key_finalfunc, 
    COMBINEFUNC = traceprov_agg_key_combine,
    SERIALFUNC = traceprov_agg_key_serialize,
    DESERIALFUNC = traceprov_agg_key_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE traceprov_agg_key_parallel(int, BIGINT, BIGINT, BIGINT) (
    SFUNC = traceprov_agg_key_sfunc, 
    STYPE = internal, 
    SSPACE = 32,
    FINALFUNC = traceprov_agg_key_finalfunc, 
    COMBINEFUNC = traceprov_agg_key_combine,
    SERIALFUNC = traceprov_agg_key_serialize,
    DESERIALFUNC = traceprov_agg_key_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE traceprov_agg_key_parallel(int, BIGINT, BIGINT, BIGINT, BIGINT) (
    SFUNC = traceprov_agg_key_sfunc, 
    STYPE = internal, 
    SSPACE = 32,
    FINALFUNC = traceprov_agg_key_finalfunc, 
    COMBINEFUNC = traceprov_agg_key_combine,
    SERIALFUNC = traceprov_agg_key_serialize,
    DESERIALFUNC = traceprov_agg_key_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE traceprov_agg_key_parallel(int, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) (
    SFUNC = traceprov_agg_key_sfunc, 
    STYPE = internal, 
    SSPACE = 32,
    FINALFUNC = traceprov_agg_key_finalfunc, 
    COMBINEFUNC = traceprov_agg_key_combine,
    SERIALFUNC = traceprov_agg_key_serialize,
    DESERIALFUNC = traceprov_agg_key_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE traceprov_agg_key_parallel(int, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) (
    SFUNC = traceprov_agg_key_sfunc, 
    STYPE = internal, 
    SSPACE = 32,
    FINALFUNC = traceprov_agg_key_finalfunc, 
    COMBINEFUNC = traceprov_agg_key_combine,
    SERIALFUNC = traceprov_agg_key_serialize,
    DESERIALFUNC = traceprov_agg_key_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE traceprov_agg_key_parallel(int, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) (
    SFUNC = traceprov_agg_key_sfunc, 
    STYPE = internal, 
    SSPACE = 32,
    FINALFUNC = traceprov_agg_key_finalfunc, 
    COMBINEFUNC = traceprov_agg_key_combine,
    SERIALFUNC = traceprov_agg_key_serialize,
    DESERIALFUNC = traceprov_agg_key_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE traceprov_agg_key_parallel(int, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) (
    SFUNC = traceprov_agg_key_sfunc, 
    STYPE = internal, 
    SSPACE = 32,
    FINALFUNC = traceprov_agg_key_finalfunc, 
    COMBINEFUNC = traceprov_agg_key_combine,
    SERIALFUNC = traceprov_agg_key_serialize,
    DESERIALFUNC = traceprov_agg_key_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE traceprov_agg_key_parallel(int, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) (
    SFUNC = traceprov_agg_key_sfunc, 
    STYPE = internal, 
    SSPACE = 32,
    FINALFUNC = traceprov_agg_key_finalfunc, 
    COMBINEFUNC = traceprov_agg_key_combine,
    SERIALFUNC = traceprov_agg_key_serialize,
    DESERIALFUNC = traceprov_agg_key_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE traceprov_agg_key_parallel(int, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) (
    SFUNC = traceprov_agg_key_sfunc, 
    STYPE = internal, 
    SSPACE = 32,
    FINALFUNC = traceprov_agg_key_finalfunc, 
    COMBINEFUNC = traceprov_agg_key_combine,
    SERIALFUNC = traceprov_agg_key_serialize,
    DESERIALFUNC = traceprov_agg_key_deserialize,
    PARALLEL = SAFE
);

CREATE FUNCTION traceprov_log_subquery_pk(INT, BIGINT) RETURNS boolean as '$libdir/__FILE__', 'traceprov_log_subquery_pk' LANGUAGE C PARALLEL SAFE STABLE;
CREATE FUNCTION traceprov_log_subquery_pk(INT, BIGINT, BIGINT) RETURNS boolean as '$libdir/__FILE__', 'traceprov_log_subquery_pk' LANGUAGE C PARALLEL SAFE STABLE;
CREATE FUNCTION traceprov_log_subquery_pk(INT, BIGINT, BIGINT, BIGINT) RETURNS boolean as '$libdir/__FILE__', 'traceprov_log_subquery_pk' LANGUAGE C PARALLEL SAFE STABLE;
CREATE FUNCTION traceprov_log_subquery_pk(INT, BIGINT, BIGINT, BIGINT, BIGINT) RETURNS boolean as '$libdir/__FILE__', 'traceprov_log_subquery_pk' LANGUAGE C PARALLEL SAFE STABLE;
CREATE FUNCTION traceprov_log_subquery_pk(INT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) RETURNS boolean as '$libdir/__FILE__', 'traceprov_log_subquery_pk' LANGUAGE C PARALLEL SAFE STABLE;
CREATE FUNCTION traceprov_log_subquery_pk(INT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) RETURNS boolean as '$libdir/__FILE__', 'traceprov_log_subquery_pk' LANGUAGE C PARALLEL SAFE STABLE;

CREATE FUNCTION traceprov_agg_from_ptr_sfunc(state internal, int, int, BIGINT) RETURNS internal AS '$libdir/__FILE__', 'traceprov_agg_from_ptr_sfunc' LANGUAGE C;
CREATE FUNCTION traceprov_agg_from_ptr_finalfunc(state internal) RETURNS bigint AS '$libdir/__FILE__', 'traceprov_agg_from_ptr_finalfunc' LANGUAGE C;
CREATE FUNCTION traceprov_agg_from_ptr_serialize(internal) RETURNS bytea AS '$libdir/__FILE__', 'traceprov_agg_from_ptr_serialize' LANGUAGE C;
CREATE FUNCTION traceprov_agg_from_ptr_deserialize(bytea, internal) RETURNS internal AS '$libdir/__FILE__', 'traceprov_agg_from_ptr_deserialize' LANGUAGE C;
CREATE FUNCTION traceprov_agg_from_ptr_combine(internal, internal) RETURNS internal AS '$libdir/__FILE__', 'traceprov_agg_from_ptr_combine' LANGUAGE C;

CREATE AGGREGATE traceprov_agg_from_ptr(int, int, BIGINT) (
    SFUNC = traceprov_agg_from_ptr_sfunc, 
    STYPE = internal, 
    SSPACE = 32,
    FINALFUNC = traceprov_agg_from_ptr_finalfunc, 
    COMBINEFUNC = traceprov_agg_from_ptr_combine,
    SERIALFUNC = traceprov_agg_from_ptr_serialize,
    DESERIALFUNC = traceprov_agg_from_ptr_deserialize,
    PARALLEL = SAFE
);