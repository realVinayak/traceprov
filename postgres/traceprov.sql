DROP FUNCTION   IF EXISTS reinit_state(INTEGER); -- This is here for historical reasons.
DROP FUNCTION   IF EXISTS reinit_state();
CREATE FUNCTION reinit_state() RETURNS INTEGER AS '$libdir/__FILE__', 'reinit_state' LANGUAGE C;

DROP FUNCTION   IF EXISTS test_local_setup(INTEGER, INTEGER);
CREATE FUNCTION test_local_setup(INTEGER, INTEGER) RETURNS INTEGER AS '$libdir/__FILE__', 'test_local_setup' LANGUAGE C;