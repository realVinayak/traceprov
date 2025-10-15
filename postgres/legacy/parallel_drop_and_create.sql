-- AGG MAP PARALLEL
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

-- AGG MAP PARALLEL
DROP AGGREGATE  IF EXISTS agg_map_parallel_second(BIGINT);
DROP AGGREGATE  IF EXISTS agg_map_parallel_second(BIGINT, BIGINT);
DROP AGGREGATE  IF EXISTS agg_map_parallel_second(BIGINT, BIGINT, BIGINT);
DROP AGGREGATE  IF EXISTS agg_map_parallel_second(BIGINT, BIGINT, BIGINT, BIGINT);
DROP AGGREGATE  IF EXISTS agg_map_parallel_second(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP AGGREGATE  IF EXISTS agg_map_parallel_second(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP AGGREGATE  IF EXISTS agg_map_parallel_second(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP AGGREGATE  IF EXISTS agg_map_parallel_second(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP AGGREGATE  IF EXISTS agg_map_parallel_second(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP AGGREGATE  IF EXISTS agg_map_parallel_second(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);


-- AGG MAP PARALLEL TAG
DROP AGGREGATE  IF EXISTS agg_map_parallel_tag(BIGINT);
DROP AGGREGATE  IF EXISTS agg_map_parallel_tag(BIGINT, BIGINT);
DROP AGGREGATE  IF EXISTS agg_map_parallel_tag(BIGINT, BIGINT, BIGINT);
DROP AGGREGATE  IF EXISTS agg_map_parallel_tag(BIGINT, BIGINT, BIGINT, BIGINT);
DROP AGGREGATE  IF EXISTS agg_map_parallel_tag(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP AGGREGATE  IF EXISTS agg_map_parallel_tag(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP AGGREGATE  IF EXISTS agg_map_parallel_tag(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP AGGREGATE  IF EXISTS agg_map_parallel_tag(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP AGGREGATE  IF EXISTS agg_map_parallel_tag(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP AGGREGATE  IF EXISTS agg_map_parallel_tag(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);

-- AGG MAP PARALLLEL (SFUNC)
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


-- AGG MAP PARALLLEL (SFUNC)
DROP FUNCTION   IF EXISTS agg_map_parallel_sfunc_second(internal, BIGINT);
DROP FUNCTION   IF EXISTS agg_map_parallel_sfunc_second(internal, BIGINT, BIGINT);
DROP FUNCTION   IF EXISTS agg_map_parallel_sfunc_second(internal, BIGINT, BIGINT, BIGINT);
DROP FUNCTION   IF EXISTS agg_map_parallel_sfunc_second(internal, BIGINT, BIGINT, BIGINT, BIGINT);
DROP FUNCTION   IF EXISTS agg_map_parallel_sfunc_second(internal, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP FUNCTION   IF EXISTS agg_map_parallel_sfunc_second(internal, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP FUNCTION   IF EXISTS agg_map_parallel_sfunc_second(internal, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP FUNCTION   IF EXISTS agg_map_parallel_sfunc_second(internal, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP FUNCTION   IF EXISTS agg_map_parallel_sfunc_second(internal, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP FUNCTION   IF EXISTS agg_map_parallel_sfunc_second(internal, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);

DROP FUNCTION   IF EXISTS agg_map_parallel_finalfunc(internal);
DROP FUNCTION   IF EXISTS agg_map_parallel_finalfunc_tag(internal);
DROP FUNCTION   IF EXISTS agg_map_parallel_combine(internal, internal);
DROP FUNCTION   IF EXISTS agg_map_parallel_serialize(internal);
DROP FUNCTION   IF EXISTS agg_map_parallel_deserialize(bytea, internal);
DROP FUNCTION   IF EXISTS reinit_state(INTEGER);

DROP FUNCTION   IF EXISTS log_subquery_pk(BIGINT, BIGINT);
DROP FUNCTION   IF EXISTS log_subquery_pk(BIGINT, BIGINT, BIGINT);
DROP FUNCTION   IF EXISTS log_subquery_pk(BIGINT, BIGINT, BIGINT, BIGINT);
DROP FUNCTION   IF EXISTS log_subquery_pk(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP FUNCTION   IF EXISTS log_subquery_pk(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);

DROP FUNCTION   IF EXISTS log_subquery_pk_neg(BIGINT, BIGINT);
DROP FUNCTION   IF EXISTS log_subquery_pk_neg(BIGINT, BIGINT, BIGINT);
DROP FUNCTION   IF EXISTS log_subquery_pk_neg(BIGINT, BIGINT, BIGINT, BIGINT);
DROP FUNCTION   IF EXISTS log_subquery_pk_neg(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);
DROP FUNCTION   IF EXISTS log_subquery_pk_neg(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT);

DROP AGGREGATE  IF EXISTS agg_map(BIGINT);
DROP FUNCTION   IF EXISTS agg_map_sfunc(state internal, bigint);
DROP FUNCTION   IF EXISTS agg_map_finalfunc(state internal);
DROP FUNCTION   IF EXISTS mark_later(bigint);
DROP FUNCTION   IF EXISTS dump_state(INTEGER);

DROP AGGREGATE  IF EXISTS agg_from_ptr(BIGINT);
DROP AGGREGATE  IF EXISTS agg_from_ptr_tag(BIGINT);
DROP FUNCTION   IF EXISTS agg_from_ptr_sfunc(internal, BIGINT);
DROP FUNCTION   IF EXISTS agg_from_ptr_sfunc_tag(internal, BIGINT);
DROP FUNCTION   IF EXISTS agg_from_ptr_combine(internal, internal);
DROP FUNCTION   IF EXISTS agg_from_ptr_serialize(internal);
DROP FUNCTION   IF EXISTS agg_from_ptr_deserialize(bytea, internal);
DROP FUNCTION   IF EXISTS agg_from_ptr_finalfunc(internal);

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

CREATE FUNCTION agg_map_parallel_sfunc_second(internal, BIGINT) RETURNS internal AS '$libdir/__FILE__', 'agg_map_parallel_sfunc_second' LANGUAGE C;
CREATE FUNCTION agg_map_parallel_sfunc_second(internal, BIGINT, BIGINT) RETURNS internal AS '$libdir/__FILE__', 'agg_map_parallel_sfunc_second' LANGUAGE C;
CREATE FUNCTION agg_map_parallel_sfunc_second(internal, BIGINT, BIGINT, BIGINT) RETURNS internal AS '$libdir/__FILE__', 'agg_map_parallel_sfunc_second' LANGUAGE C;
CREATE FUNCTION agg_map_parallel_sfunc_second(internal, BIGINT, BIGINT, BIGINT, BIGINT) RETURNS internal AS '$libdir/__FILE__', 'agg_map_parallel_sfunc_second' LANGUAGE C;
CREATE FUNCTION agg_map_parallel_sfunc_second(internal, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) RETURNS internal AS '$libdir/__FILE__', 'agg_map_parallel_sfunc_second' LANGUAGE C;
CREATE FUNCTION agg_map_parallel_sfunc_second(internal, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) RETURNS internal AS '$libdir/__FILE__', 'agg_map_parallel_sfunc_second' LANGUAGE C;
CREATE FUNCTION agg_map_parallel_sfunc_second(internal, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) RETURNS internal AS '$libdir/__FILE__', 'agg_map_parallel_sfunc_second' LANGUAGE C;
CREATE FUNCTION agg_map_parallel_sfunc_second(internal, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) RETURNS internal AS '$libdir/__FILE__', 'agg_map_parallel_sfunc_second' LANGUAGE C;
CREATE FUNCTION agg_map_parallel_sfunc_second(internal, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) RETURNS internal AS '$libdir/__FILE__', 'agg_map_parallel_sfunc_second' LANGUAGE C;
CREATE FUNCTION agg_map_parallel_sfunc_second(internal, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) RETURNS internal AS '$libdir/__FILE__', 'agg_map_parallel_sfunc_second' LANGUAGE C;


CREATE FUNCTION agg_map_parallel_finalfunc(internal) RETURNS BIGINT AS '$libdir/__FILE__', 'agg_map_parallel_finalfunc' LANGUAGE C;
CREATE FUNCTION agg_map_parallel_finalfunc_tag(internal) RETURNS BIGINT AS '$libdir/__FILE__', 'agg_map_parallel_finalfunc_tag' LANGUAGE C;
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
CREATE FUNCTION log_subquery_pk(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) RETURNS boolean as '$libdir/__FILE__', 'log_subquery_pk' LANGUAGE C PARALLEL SAFE STABLE;
CREATE FUNCTION log_subquery_pk(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) RETURNS boolean as '$libdir/__FILE__', 'log_subquery_pk' LANGUAGE C PARALLEL SAFE STABLE;

CREATE FUNCTION log_subquery_pk_neg(BIGINT, BIGINT) RETURNS boolean as '$libdir/__FILE__', 'log_subquery_pk_neg' LANGUAGE C PARALLEL SAFE STABLE;
CREATE FUNCTION log_subquery_pk_neg(BIGINT, BIGINT, BIGINT) RETURNS boolean as '$libdir/__FILE__', 'log_subquery_pk_neg' LANGUAGE C PARALLEL SAFE STABLE;
CREATE FUNCTION log_subquery_pk_neg(BIGINT, BIGINT, BIGINT, BIGINT) RETURNS boolean as '$libdir/__FILE__', 'log_subquery_pk_neg' LANGUAGE C PARALLEL SAFE STABLE;
CREATE FUNCTION log_subquery_pk_neg(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) RETURNS boolean as '$libdir/__FILE__', 'log_subquery_pk_neg' LANGUAGE C PARALLEL SAFE STABLE;
CREATE FUNCTION log_subquery_pk_neg(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) RETURNS boolean as '$libdir/__FILE__', 'log_subquery_pk_neg' LANGUAGE C PARALLEL SAFE STABLE;


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



CREATE AGGREGATE agg_map_parallel_tag(BIGINT) (
    SFUNC = agg_map_parallel_sfunc,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = agg_map_parallel_finalfunc_tag,
    COMBINEFUNC = agg_map_parallel_combine,
    SERIALFUNC = agg_map_parallel_serialize,
    DESERIALFUNC = agg_map_parallel_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE agg_map_parallel_tag(BIGINT, BIGINT) (
    SFUNC = agg_map_parallel_sfunc,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = agg_map_parallel_finalfunc_tag,
    COMBINEFUNC = agg_map_parallel_combine,
    SERIALFUNC = agg_map_parallel_serialize,
    DESERIALFUNC = agg_map_parallel_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE agg_map_parallel_tag(BIGINT, BIGINT, BIGINT) (
    SFUNC = agg_map_parallel_sfunc,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = agg_map_parallel_finalfunc_tag,
    COMBINEFUNC = agg_map_parallel_combine,
    SERIALFUNC = agg_map_parallel_serialize,
    DESERIALFUNC = agg_map_parallel_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE agg_map_parallel_tag(BIGINT, BIGINT, BIGINT, BIGINT) (
    SFUNC = agg_map_parallel_sfunc,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = agg_map_parallel_finalfunc_tag,
    COMBINEFUNC = agg_map_parallel_combine,
    SERIALFUNC = agg_map_parallel_serialize,
    DESERIALFUNC = agg_map_parallel_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE agg_map_parallel_tag(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) (
    SFUNC = agg_map_parallel_sfunc,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = agg_map_parallel_finalfunc_tag,
    COMBINEFUNC = agg_map_parallel_combine,
    SERIALFUNC = agg_map_parallel_serialize,
    DESERIALFUNC = agg_map_parallel_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE agg_map_parallel_tag(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) (
    SFUNC = agg_map_parallel_sfunc,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = agg_map_parallel_finalfunc_tag,
    COMBINEFUNC = agg_map_parallel_combine,
    SERIALFUNC = agg_map_parallel_serialize,
    DESERIALFUNC = agg_map_parallel_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE agg_map_parallel_tag(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) (
    SFUNC = agg_map_parallel_sfunc,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = agg_map_parallel_finalfunc_tag,
    COMBINEFUNC = agg_map_parallel_combine,
    SERIALFUNC = agg_map_parallel_serialize,
    DESERIALFUNC = agg_map_parallel_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE agg_map_parallel_tag(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) (
    SFUNC = agg_map_parallel_sfunc,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = agg_map_parallel_finalfunc_tag,
    COMBINEFUNC = agg_map_parallel_combine,
    SERIALFUNC = agg_map_parallel_serialize,
    DESERIALFUNC = agg_map_parallel_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE agg_map_parallel_tag(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) (
    SFUNC = agg_map_parallel_sfunc,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = agg_map_parallel_finalfunc_tag,
    COMBINEFUNC = agg_map_parallel_combine,
    SERIALFUNC = agg_map_parallel_serialize,
    DESERIALFUNC = agg_map_parallel_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE agg_map_parallel_tag(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) (
    SFUNC = agg_map_parallel_sfunc,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = agg_map_parallel_finalfunc_tag,
    COMBINEFUNC = agg_map_parallel_combine,
    SERIALFUNC = agg_map_parallel_serialize,
    DESERIALFUNC = agg_map_parallel_deserialize,
    PARALLEL = SAFE
);


CREATE AGGREGATE agg_map_parallel_second(BIGINT) (
    SFUNC = agg_map_parallel_sfunc_second,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = agg_map_parallel_finalfunc,
    COMBINEFUNC = agg_map_parallel_combine,
    SERIALFUNC = agg_map_parallel_serialize,
    DESERIALFUNC = agg_map_parallel_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE agg_map_parallel_second(BIGINT, BIGINT) (
    SFUNC = agg_map_parallel_sfunc_second,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = agg_map_parallel_finalfunc,
    COMBINEFUNC = agg_map_parallel_combine,
    SERIALFUNC = agg_map_parallel_serialize,
    DESERIALFUNC = agg_map_parallel_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE agg_map_parallel_second(BIGINT, BIGINT, BIGINT) (
    SFUNC = agg_map_parallel_sfunc_second,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = agg_map_parallel_finalfunc,
    COMBINEFUNC = agg_map_parallel_combine,
    SERIALFUNC = agg_map_parallel_serialize,
    DESERIALFUNC = agg_map_parallel_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE agg_map_parallel_second(BIGINT, BIGINT, BIGINT, BIGINT) (
    SFUNC = agg_map_parallel_sfunc_second,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = agg_map_parallel_finalfunc,
    COMBINEFUNC = agg_map_parallel_combine,
    SERIALFUNC = agg_map_parallel_serialize,
    DESERIALFUNC = agg_map_parallel_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE agg_map_parallel_second(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) (
    SFUNC = agg_map_parallel_sfunc_second,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = agg_map_parallel_finalfunc,
    COMBINEFUNC = agg_map_parallel_combine,
    SERIALFUNC = agg_map_parallel_serialize,
    DESERIALFUNC = agg_map_parallel_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE agg_map_parallel_second(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) (
    SFUNC = agg_map_parallel_sfunc_second,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = agg_map_parallel_finalfunc,
    COMBINEFUNC = agg_map_parallel_combine,
    SERIALFUNC = agg_map_parallel_serialize,
    DESERIALFUNC = agg_map_parallel_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE agg_map_parallel_second(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) (
    SFUNC = agg_map_parallel_sfunc_second,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = agg_map_parallel_finalfunc,
    COMBINEFUNC = agg_map_parallel_combine,
    SERIALFUNC = agg_map_parallel_serialize,
    DESERIALFUNC = agg_map_parallel_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE agg_map_parallel_second(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) (
    SFUNC = agg_map_parallel_sfunc_second,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = agg_map_parallel_finalfunc,
    COMBINEFUNC = agg_map_parallel_combine,
    SERIALFUNC = agg_map_parallel_serialize,
    DESERIALFUNC = agg_map_parallel_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE agg_map_parallel_second(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) (
    SFUNC = agg_map_parallel_sfunc_second,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = agg_map_parallel_finalfunc,
    COMBINEFUNC = agg_map_parallel_combine,
    SERIALFUNC = agg_map_parallel_serialize,
    DESERIALFUNC = agg_map_parallel_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE agg_map_parallel_second(BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT, BIGINT) (
    SFUNC = agg_map_parallel_sfunc_second,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = agg_map_parallel_finalfunc,
    COMBINEFUNC = agg_map_parallel_combine,
    SERIALFUNC = agg_map_parallel_serialize,
    DESERIALFUNC = agg_map_parallel_deserialize,
    PARALLEL = SAFE
);


CREATE FUNCTION agg_from_ptr_sfunc(internal, BIGINT) RETURNS internal AS '$libdir/__FILE__', 'agg_from_ptr_sfunc' LANGUAGE C;
CREATE FUNCTION agg_from_ptr_sfunc_tag(internal, BIGINT) RETURNS internal AS '$libdir/__FILE__', 'agg_from_ptr_sfunc_tag' LANGUAGE C;

CREATE FUNCTION agg_from_ptr_finalfunc(internal) RETURNS BIGINT AS '$libdir/__FILE__', 'agg_from_ptr_finalfunc' LANGUAGE C;
CREATE FUNCTION agg_from_ptr_combine(internal, internal) RETURNS internal AS '$libdir/__FILE__', 'agg_from_ptr_combine' LANGUAGE C;
CREATE FUNCTION agg_from_ptr_serialize(internal) RETURNS bytea AS '$libdir/__FILE__', 'agg_from_ptr_serialize' LANGUAGE C;
CREATE FUNCTION agg_from_ptr_deserialize(bytea, internal) RETURNS internal AS '$libdir/__FILE__', 'agg_from_ptr_deserialize' LANGUAGE C;

CREATE AGGREGATE agg_from_ptr(BIGINT) (
    SFUNC = agg_from_ptr_sfunc,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = agg_from_ptr_finalfunc,
    COMBINEFUNC = agg_from_ptr_combine,
    SERIALFUNC = agg_from_ptr_serialize,
    DESERIALFUNC = agg_from_ptr_deserialize,
    PARALLEL = SAFE
);

CREATE AGGREGATE agg_from_ptr_tag(BIGINT) (
    SFUNC = agg_from_ptr_sfunc_tag,
    STYPE = internal,
    SSPACE = 32,
    FINALFUNC = agg_from_ptr_finalfunc,
    COMBINEFUNC = agg_from_ptr_combine,
    SERIALFUNC = agg_from_ptr_serialize,
    DESERIALFUNC = agg_from_ptr_deserialize,
    PARALLEL = SAFE
);