CREATE OR REPLACE FUNCTION traceprov_infer_45aaa943(
    -- These 3 args will always be given.
    IN integer,
    IN integer, 
    IN integer,
    OUT p_partkey INTEGER,OUT s_suppkey INTEGER,OUT n_nationkey INTEGER,OUT r_regionkey INTEGER,OUT ps_suppkey INTEGER,OUT ps_partkey INTEGER
    )
    RETURNS SETOF record
    AS '$libdir/__FILE__', 'traceprov_infer'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;