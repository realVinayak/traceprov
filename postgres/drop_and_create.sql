DROP AGGREGATE if exists agg_map(BIGINT);
DROP FUNCTION if exists agg_map_sfunc(state internal, bigint);
DROP FUNCTION if exists agg_map_finalfunc(state internal);
DROP FUNCTION if exists reinit_state(INTEGER);
DROP FUNCTION if exists mark_later(bigint);
DROP FUNCTION if exists dump_state(INTEGER);

CREATE FUNCTION reinit_state(INTEGER) RETURNS INTEGER AS '$libdir/test_udfs_2_34', 'reinit_state' LANGUAGE C;
CREATE FUNCTION dump_state(INTEGER) RETURNS INTEGER AS '$libdir/test_udfs_2_34', 'dump_state' LANGUAGE C;

CREATE FUNCTION mark_later(bigint) RETURNS INTEGER AS '$libdir/test_udfs_2_34', 'mark_later' LANGUAGE C;

CREATE FUNCTION agg_map_sfunc(state internal, bigint) RETURNS internal AS '$libdir/test_udfs_2_34', 'agg_map_sfunc' LANGUAGE C;
CREATE FUNCTION agg_map_finalfunc(state internal) RETURNS bigint AS '$libdir/test_udfs_2_34', 'agg_map_finalfunc' LANGUAGE C;
CREATE AGGREGATE agg_map(BIGINT) ( SFUNC = agg_map_sfunc, STYPE = internal, FINALFUNC = agg_map_finalfunc, SSPACE = 128 );