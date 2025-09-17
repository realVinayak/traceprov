CREATE OR REPLACE FUNCTION traceprov_infer(IN integer, IN integer, IN integer,
    OUT f1 integer, OUT f2 integer, OUT f3 integer, OUT f4 integer)
    RETURNS SETOF record
    AS '$libdir/__FILE__', 'traceprov_infer'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION traceprov_infer_count(IN integer, IN bigint)
    RETURNS bigint
    AS '$libdir/__FILE__', 'traceprov_infer_count'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION traceprov_infer_poly(IN integer, IN bigint, IN integer, IN integer, IN integer)
    RETURNS text
    AS '$libdir/__FILE__', 'traceprov_infer_poly'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;