CREATE OR REPLACE FUNCTION traceprov_infer_2fbc5273(
    -- These 3 args will always be given.
    IN integer,
    IN integer, 
    IN integer,
    OUT ps_suppkey INTEGER,OUT ps_partkey INTEGER,OUT s_suppkey INTEGER,OUT n_nationkey INTEGER
    )
    RETURNS SETOF record
    AS '$libdir/__FILE__', 'traceprov_infer'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;