CREATE OR REPLACE FUNCTION traceprov_infer_selectivity_bench(
    -- These 3 args will always be given.
    IN integer,
    IN integer, 
    IN integer,
    OUT id integer,
    OUT sample_integer integer
    )
    RETURNS SETOF record
    AS '$libdir/__FILE__', 'traceprov_infer'
    LANGUAGE C STRICT PARALLEL SAFE;