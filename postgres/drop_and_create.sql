DROP AGGREGATE agg_map(BIGINT);
DROP FUNCTION agg_map_sfunc(state internal, bigint);
DROP FUNCTION agg_map_finalfunc(state internal);
DROP FUNCTION reinit_state(INTEGER);

CREATE FUNCTION reinit_state(INTEGER) RETURNS INTEGER AS '$libdir/test_udfs_2_14', 'reinit_state' LANGUAGE C;
CREATE FUNCTION agg_map_sfunc(state internal, bigint) RETURNS internal AS '$libdir/test_udfs_2_14', 'agg_map_sfunc' LANGUAGE C;
CREATE FUNCTION agg_map_finalfunc(state internal) RETURNS bigint AS '$libdir/test_udfs_2_14', 'agg_map_finalfunc' LANGUAGE C;
CREATE AGGREGATE agg_map(BIGINT) ( SFUNC = agg_map_sfunc, STYPE = internal, FINALFUNC = agg_map_finalfunc, SSPACE = 32 );