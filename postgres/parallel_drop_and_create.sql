DROP AGGREGATE  IF EXISTS agg_map_parallel(BIGINT);
DROP AGGREGATE  IF EXISTS agg_map_parallel(BIGINT, BIGINT);
DROP AGGREGATE  IF EXISTS agg_map_parallel(BIGINT, BIGINT, BIGINT);
DROP AGGREGATE  IF EXISTS agg_map_parallel(BIGINT, BIGINT, BIGINT, BIGINT);
DROP AGGREGATE  IF EXISTS agg_map_parallel(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP AGGREGATE  IF EXISTS agg_map_parallel(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP AGGREGATE  IF EXISTS agg_map_parallel(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP AGGREGATE  IF EXISTS agg_map_parallel(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP AGGREGATE  IF EXISTS agg_map_parallel(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP AGGREGATE  IF EXISTS agg_map_parallel(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);


DROP FUNCTION   IF EXISTS agg_map_parallel_sfunc(internal, BIGINT);
DROP FUNCTION   IF EXISTS agg_map_parallel_sfunc(internal, BIGINT, BIGINT);
DROP FUNCTION   IF EXISTS agg_map_parallel_sfunc(internal, BIGINT, BIGINT, BIGINT);
DROP FUNCTION   IF EXISTS agg_map_parallel_sfunc(internal, BIGINT, BIGINT, BIGINT, BIGINT);
DROP FUNCTION   IF EXISTS agg_map_parallel_sfunc(internal, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP FUNCTION   IF EXISTS agg_map_parallel_sfunc(internal, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP FUNCTION   IF EXISTS agg_map_parallel_sfunc(internal, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP FUNCTION   IF EXISTS agg_map_parallel_sfunc(internal, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP FUNCTION   IF EXISTS agg_map_parallel_sfunc(internal, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP FUNCTION   IF EXISTS agg_map_parallel_sfunc(internal, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);

DROP FUNCTION   IF EXISTS agg_map_parallel_finalfunc(internal);
DROP FUNCTION   IF EXISTS agg_map_parallel_combine(internal, internal);
DROP FUNCTION   IF EXISTS agg_map_parallel_serialize(internal);
DROP FUNCTION   IF EXISTS agg_map_parallel_deserialize(bytea, internal);
DROP FUNCTION   IF EXISTS reinit_state(INTEGER);

DROP FUNCTION   IF EXISTS log_subquery_pk(BIGINT, BIGINT);
DROP FUNCTION   IF EXISTS log_subquery_pk(BIGINT, BIGINT, BIGINT);
DROP FUNCTION   IF EXISTS log_subquery_pk(BIGINT, BIGINT, BIGINT, BIGINT);


DROP AGGREGATE  IF EXISTS agg_map(BIGINT);
DROP FUNCTION   IF EXISTS agg_map_sfunc(state internal, bigint);
DROP FUNCTION   IF EXISTS agg_map_finalfunc(state internal);
DROP FUNCTION   IF EXISTS mark_later(bigint);
DROP FUNCTION   IF EXISTS dump_state(INTEGER);


CREATE FUNCTION agg_map_parallel_sfunc(internal, BIGINT) RETURNS internal AS '$libdir/__FILE__', 'agg_map_parallel_sfunc' LANGUAGE C;
CREATE FUNCTION agg_map_parallel_sfunc(internal, BIGINT, BIGINT) RETURNS internal AS '$libdir/__FILE__', 'agg_map_parallel_sfunc' LANGUAGE C;
CREATE FUNCTION agg_map_parallel_sfunc(internal, BIGINT, BIGINT, BIGINT) RETURNS internal AS '$libdir/__FILE__', 'agg_map_parallel_sfunc' LANGUAGE C;
CREATE FUNCTION agg_map_parallel_sfunc(internal, BIGINT, BIGINT, BIGINT, BIGINT) RETURNS internal AS '$libdir/__FILE__', 'agg_map_parallel_sfunc' LANGUAGE C;
CREATE FUNCTION agg_map_parallel_sfunc(internal, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) RETURNS internal AS '$libdir/__FILE__', 'agg_map_parallel_sfunc' LANGUAGE C;
CREATE FUNCTION agg_map_parallel_sfunc(internal, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) RETURNS internal AS '$libdir/__FILE__', 'agg_map_parallel_sfunc' LANGUAGE C;
CREATE FUNCTION agg_map_parallel_sfunc(internal, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) RETURNS internal AS '$libdir/__FILE__', 'agg_map_parallel_sfunc' LANGUAGE C;
CREATE FUNCTION agg_map_parallel_sfunc(internal, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) RETURNS internal AS '$libdir/__FILE__', 'agg_map_parallel_sfunc' LANGUAGE C;
CREATE FUNCTION agg_map_parallel_sfunc(internal, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) RETURNS internal AS '$libdir/__FILE__', 'agg_map_parallel_sfunc' LANGUAGE C;
CREATE FUNCTION agg_map_parallel_sfunc(internal, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) RETURNS internal AS '$libdir/__FILE__', 'agg_map_parallel_sfunc' LANGUAGE C;


CREATE FUNCTION agg_map_parallel_finalfunc(internal) RETURNS BIGINT AS '$libdir/__FILE__', 'agg_map_parallel_finalfunc' LANGUAGE C;
CREATE FUNCTION agg_map_parallel_combine(internal, internal) RETURNS internal AS '$libdir/__FILE__', 'agg_map_parallel_combine' LANGUAGE C;
CREATE FUNCTION agg_map_parallel_serialize(internal) RETURNS bytea AS '$libdir/__FILE__', 'agg_map_parallel_serialize' LANGUAGE C;
CREATE FUNCTION agg_map_parallel_deserialize(bytea, internal) RETURNS internal AS '$libdir/__FILE__', 'agg_map_parallel_deserialize' LANGUAGE C;


CREATE FUNCTION mark_later(bigint) RETURNS INTEGER AS '$libdir/__FILE__', 'mark_later' LANGUAGE C PARALLEL SAFE;

CREATE FUNCTION agg_map_sfunc(state internal, bigint) RETURNS internal AS '$libdir/__FILE__', 'agg_map_sfunc' LANGUAGE C;
CREATE FUNCTION agg_map_finalfunc(state internal) RETURNS bigint AS '$libdir/__FILE__', 'agg_map_finalfunc' LANGUAGE C;


CREATE FUNCTION reinit_state(INTEGER) RETURNS INTEGER AS '$libdir/__FILE__', 'reinit_state' LANGUAGE C;

CREATE FUNCTION log_subquery_pk(BIGINT, BIGINT) RETURNS boolean as '$libdir/__FILE__', 'log_subquery_pk' LANGUAGE C PARALLEL SAFE STABLE;
CREATE FUNCTION log_subquery_pk(BIGINT, BIGINT, BIGINT) RETURNS boolean as '$libdir/__FILE__', 'log_subquery_pk' LANGUAGE C PARALLEL SAFE STABLE;
CREATE FUNCTION log_subquery_pk(BIGINT, BIGINT, BIGINT, BIGINT) RETURNS boolean as '$libdir/__FILE__', 'log_subquery_pk' LANGUAGE C PARALLEL SAFE STABLE;


CREATE AGGREGATE agg_map_parallel(BIGINT) (
    SFUNC = agg_map_parallel_sfunc,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = agg_map_parallel_finalfunc,
    COMBINEFUNC = agg_map_parallel_combine,
    SERIALFUNC = agg_map_parallel_serialize,
    DESERIALFUNC = agg_map_parallel_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE agg_map_parallel(BIGINT, BIGINT) (
    SFUNC = agg_map_parallel_sfunc,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = agg_map_parallel_finalfunc,
    COMBINEFUNC = agg_map_parallel_combine,
    SERIALFUNC = agg_map_parallel_serialize,
    DESERIALFUNC = agg_map_parallel_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE agg_map_parallel(BIGINT, BIGINT, BIGINT) (
    SFUNC = agg_map_parallel_sfunc,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = agg_map_parallel_finalfunc,
    COMBINEFUNC = agg_map_parallel_combine,
    SERIALFUNC = agg_map_parallel_serialize,
    DESERIALFUNC = agg_map_parallel_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE agg_map_parallel(BIGINT, BIGINT, BIGINT, BIGINT) (
    SFUNC = agg_map_parallel_sfunc,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = agg_map_parallel_finalfunc,
    COMBINEFUNC = agg_map_parallel_combine,
    SERIALFUNC = agg_map_parallel_serialize,
    DESERIALFUNC = agg_map_parallel_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE agg_map_parallel(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) (
    SFUNC = agg_map_parallel_sfunc,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = agg_map_parallel_finalfunc,
    COMBINEFUNC = agg_map_parallel_combine,
    SERIALFUNC = agg_map_parallel_serialize,
    DESERIALFUNC = agg_map_parallel_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE agg_map_parallel(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) (
    SFUNC = agg_map_parallel_sfunc,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = agg_map_parallel_finalfunc,
    COMBINEFUNC = agg_map_parallel_combine,
    SERIALFUNC = agg_map_parallel_serialize,
    DESERIALFUNC = agg_map_parallel_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE agg_map_parallel(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) (
    SFUNC = agg_map_parallel_sfunc,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = agg_map_parallel_finalfunc,
    COMBINEFUNC = agg_map_parallel_combine,
    SERIALFUNC = agg_map_parallel_serialize,
    DESERIALFUNC = agg_map_parallel_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE agg_map_parallel(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) (
    SFUNC = agg_map_parallel_sfunc,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = agg_map_parallel_finalfunc,
    COMBINEFUNC = agg_map_parallel_combine,
    SERIALFUNC = agg_map_parallel_serialize,
    DESERIALFUNC = agg_map_parallel_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE agg_map_parallel(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) (
    SFUNC = agg_map_parallel_sfunc,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = agg_map_parallel_finalfunc,
    COMBINEFUNC = agg_map_parallel_combine,
    SERIALFUNC = agg_map_parallel_serialize,
    DESERIALFUNC = agg_map_parallel_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE agg_map_parallel(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) (
    SFUNC = agg_map_parallel_sfunc,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = agg_map_parallel_finalfunc,
    COMBINEFUNC = agg_map_parallel_combine,
    SERIALFUNC = agg_map_parallel_serialize,
    DESERIALFUNC = agg_map_parallel_deserialize,
    PARALLEL = SAFE
);
-- CREATE AGGREGATE agg_map(BIGINT) ( SFUNC = agg_map_sfunc, STYPE = internal, FINALFUNC = agg_map_finalfunc, SSPACE = 128 );