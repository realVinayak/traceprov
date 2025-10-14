CREATE OR REPLACE FUNCTION traceprov_infer_2a05712d(
    -- These 3 args will always be given.
    IN integer,
    IN integer, 
    IN integer,
    OUT c_custkey INTEGER,OUT o_orderkey INTEGER
    )
    RETURNS SETOF record
    AS '$libdir/__FILE__', 'traceprov_infer'
    LANGUAGE C STRICT PARALLEL SAFE;