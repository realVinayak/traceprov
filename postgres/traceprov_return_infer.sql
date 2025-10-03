DROP FUNCTION IF EXISTS traceprov_infer(integer,integer,integer);
DROP FUNCTION IF EXISTS traceprov_infer_time(integer,integer,integer);
DROP FUNCTION IF EXISTS traceprov_sync_time(integer,integer,integer);

CREATE OR REPLACE FUNCTION traceprov_infer(IN integer, IN integer, IN integer,
    OUT f1 integer, OUT f2 integer)
    RETURNS SETOF record
    AS '$libdir/__FILE__', 'traceprov_infer'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION traceprov_infer_time(IN integer, IN integer, IN integer)
    RETURNS BIGINT
    AS '$libdir/__FILE__', 'traceprov_infer_time'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION traceprov_sync_time(IN integer)
    RETURNS BIGINT
    AS '$libdir/__FILE__', 'traceprov_sync_time'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;