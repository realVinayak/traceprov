CREATE OR REPLACE FUNCTION traceprov_infer_f9f201b9(
    -- These 3 args will always be given.
    IN integer,
    IN integer, 
    IN integer,
    OUT c_custkey INTEGER,OUT o_orderkey INTEGER,OUT l_orderkey INTEGER,OUT l_linenumber INTEGER
    )
    RETURNS SETOF record
    AS '$libdir/__FILE__', 'traceprov_infer'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;