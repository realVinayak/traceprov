CREATE FUNCTION add_one(integer) RETURNS integer
     AS '$libdir/test_func', 'add_one'
     LANGUAGE C STRICT;

