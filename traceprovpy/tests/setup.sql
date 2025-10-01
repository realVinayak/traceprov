CREATE TABLE test_table (a INT, b INT);
INSERT INTO test_table (a, b) VALUES (1, 1), (1, 3), (2, 5), (8, 9);

CREATE FUNCTION reinit_state() RETURNS INTEGER AS $$
BEGIN
    RETURN 0;
END;
$$ LANGUAGE plpgsql;
