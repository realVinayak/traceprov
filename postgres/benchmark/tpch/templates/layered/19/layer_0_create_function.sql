CREATE OR REPLACE FUNCTION traceprov_infer_c356c70c(
    -- These 3 args will always be given.
    IN integer,
    IN integer, 
    IN integer,
    OUT l_orderkey INTEGER,OUT l_linenumber INTEGER,OUT p_partkey INTEGER
    )
    RETURNS SETOF record
    AS '$libdir/__FILE__', 'traceprov_infer'
    LANGUAGE C STRICT PARALLEL SAFE;