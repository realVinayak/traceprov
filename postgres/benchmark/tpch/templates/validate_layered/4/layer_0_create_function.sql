CREATE OR REPLACE FUNCTION traceprov_infer_38dcd1ce(
    -- These 3 args will always be given.
    IN integer,
    IN integer, 
    IN integer,
    OUT o_orderkey INTEGER,OUT sample_column INTEGER
    )
    RETURNS SETOF record
    AS '$libdir/__FILE__', 'traceprov_infer'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;