CREATE OR REPLACE FUNCTION traceprov_infer_73763f6d(
    -- These 3 args will always be given.
    IN integer,
    IN integer, 
    IN integer,
    OUT l_orderkey INTEGER,OUT l_linenumber INTEGER
    )
    RETURNS SETOF record
    AS '$libdir/__FILE__', 'traceprov_infer'
    LANGUAGE C STRICT PARALLEL SAFE;