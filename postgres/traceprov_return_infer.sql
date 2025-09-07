CREATE OR REPLACE FUNCTION traceprov_infer(IN integer, IN integer, IN integer,
    OUT f1 integer, OUT f2 integer, OUT f3 integer, OUT f4 integer)
    RETURNS SETOF record
    AS '$libdir/__FILE__', 'traceprov_infer'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;