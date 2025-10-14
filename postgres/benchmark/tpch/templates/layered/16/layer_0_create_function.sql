CREATE OR REPLACE FUNCTION traceprov_infer_27b3792c(
    -- These 3 args will always be given.
    IN integer,
    IN integer, 
    IN integer,
    OUT ps_partkey INTEGER,OUT ps_suppkey INTEGER,OUT p_partkey INTEGER
    )
    RETURNS SETOF record
    AS '$libdir/__FILE__', 'traceprov_infer'
    LANGUAGE C STRICT PARALLEL SAFE;