CREATE OR REPLACE FUNCTION traceprov_infer_e2a255ee(
    -- These 3 args will always be given.
    IN integer,
    IN integer, 
    IN integer,
    OUT p_partkey INTEGER,OUT l_orderkey INTEGER,OUT l_linenumber INTEGER,OUT scratch_0 INTEGER
    )
    RETURNS SETOF record
    AS '$libdir/__FILE__', 'traceprov_infer'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;