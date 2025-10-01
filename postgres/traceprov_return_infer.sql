CREATE OR REPLACE FUNCTION traceprov_infer(IN integer, IN integer, IN integer,
    OUT f1 integer, OUT f2 integer, OUT f3 integer, OUT f4 integer)
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