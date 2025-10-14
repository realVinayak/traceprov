CREATE OR REPLACE FUNCTION traceprov_infer_4b719ae7(
    -- These 3 args will always be given.
    IN integer,
    IN integer, 
    IN integer,
    OUT ps_suppkey INTEGER,OUT ps_partkey INTEGER,OUT s_suppkey INTEGER,OUT n_nationkey INTEGER,OUT r_regionkey INTEGER
    )
    RETURNS SETOF record
    AS '$libdir/__FILE__', 'traceprov_infer'
    LANGUAGE C STRICT PARALLEL SAFE;