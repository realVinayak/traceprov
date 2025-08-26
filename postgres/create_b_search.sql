DROP FUNCTION custom_binary_search(bigint[], bigint);
create function custom_binary_search (bigint[], bigint) returns bigint AS '$libdir/__FILE__', 'binary_search' LANGUAGE C PARALLEL SAFE;
DROP FUNCTION custom_binary_search_value(bigint[], bigint);
create function custom_binary_search_value (bigint[], bigint) returns bigint AS '$libdir/__FILE__', 'binary_search_value' LANGUAGE C PARALLEL SAFE;