CREATE OR REPLACE FUNCTION traceprov_infer_152beb08(
    -- These 3 args will always be given.
    IN integer,
    IN integer, 
    IN integer,
    OUT o_orderkey INTEGER,OUT sample_column INTEGER
    )
    RETURNS SETOF record
    AS '$libdir/__FILE__', 'traceprov_infer'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;