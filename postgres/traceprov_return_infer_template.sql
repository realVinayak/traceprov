CREATE OR REPLACE FUNCTION traceprov_infer_%A%(
    -- These 3 args will always be given.
    IN integer,
    IN integer, 
    IN integer,
    %OUT%
    )
    RETURNS SETOF record
    AS '$libdir/__FILE__', 'traceprov_infer'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;