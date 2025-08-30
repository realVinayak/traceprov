DROP FUNCTION   IF EXISTS reinit_state(INTEGER); -- This is here for historical reasons.
DROP FUNCTION   IF EXISTS reinit_state();
DROP FUNCTION   IF EXISTS mark_later(bigint);
CREATE FUNCTION reinit_state() RETURNS INTEGER AS '$libdir/__FILE__', 'reinit_state' LANGUAGE C;

DROP FUNCTION   IF EXISTS test_local_setup(INTEGER, INTEGER);
CREATE FUNCTION test_local_setup(INTEGER, INTEGER) RETURNS INTEGER AS '$libdir/__FILE__', 'test_local_setup' LANGUAGE C;

DROP AGGREGATE traceprov_agg_key(BIGINT);
DROP FUNCTION   IF EXISTS traceprov_agg_key_sfunc(state internal, bigint);
DROP AGGREGATE traceprov_agg_key(int, BIGINT);
DROP FUNCTION   IF EXISTS traceprov_agg_key_sfunc(state internal, int, bigint);
DROP FUNCTION   IF EXISTS traceprov_agg_key_finalfunc(state internal);

CREATE FUNCTION traceprov_agg_key_sfunc(state internal, int, bigint) RETURNS internal AS '$libdir/__FILE__', 'traceprov_agg_key_sfunc' LANGUAGE C;
CREATE FUNCTION traceprov_agg_key_finalfunc(state internal) RETURNS bigint AS '$libdir/__FILE__', 'traceprov_agg_key_finalfunc' LANGUAGE C;
CREATE AGGREGATE traceprov_agg_key(int, BIGINT) (SFUNC = traceprov_agg_key_sfunc, STYPE = internal, FINALFUNC = traceprov_agg_key_finalfunc, SSPACE = 32);

CREATE FUNCTION mark_later(bigint) RETURNS INTEGER AS '$libdir/__FILE__', 'mark_later' LANGUAGE C PARALLEL SAFE;