-- Aggregates.
DROP AGGREGATE traceprov_agg_key(BIGINT);
DROP AGGREGATE traceprov_agg_key(int, BIGINT);
DROP AGGREGATE traceprov_agg_key_parallel(int, BIGINT);

-- Functions
DROP FUNCTION   IF EXISTS reinit_state(INTEGER); -- This is here for historical reasons.
DROP FUNCTION   IF EXISTS reinit_state();
DROP FUNCTION   IF EXISTS mark_later(bigint);
DROP FUNCTION   IF EXISTS test_local_setup(INTEGER, INTEGER);
DROP FUNCTION   IF EXISTS traceprov_agg_key_sfunc(state internal, bigint);
DROP FUNCTION   IF EXISTS traceprov_agg_key_sfunc(state internal, int, bigint);
DROP FUNCTION   IF EXISTS traceprov_agg_key_finalfunc(state internal);

-- serialize and deserialize
DROP FUNCTION   IF EXISTS traceprov_agg_key_serialize(internal);
DROP FUNCTION   IF EXISTS traceprov_agg_key_deserialize(bytea, internal);

-- combine
DROP FUNCTION   IF EXISTS traceprov_agg_key_combine(internal, internal);

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