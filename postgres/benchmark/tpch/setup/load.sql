COPY part FROM '/home/postgres/tpch_01_2025_08_16/part.csv' WITH (FORMAT csv, DELIMITER '|');
COPY region FROM '/home/postgres/tpch_01_2025_08_16/region.csv' WITH (FORMAT csv, DELIMITER '|');
COPY nation FROM '/home/postgres/tpch_01_2025_08_16/nation.csv' WITH (FORMAT csv, DELIMITER '|');
COPY supplier FROM '/home/postgres/tpch_01_2025_08_16/supplier.csv' WITH (FORMAT csv, DELIMITER '|');
COPY customer FROM '/home/postgres/tpch_01_2025_08_16/customer.csv' WITH (FORMAT csv, DELIMITER '|');
COPY partsupp FROM '/home/postgres/tpch_01_2025_08_16/partsupp.csv' WITH (FORMAT csv, DELIMITER '|');
COPY orders FROM '/home/postgres/tpch_01_2025_08_16/orders.csv' WITH (FORMAT csv, DELIMITER '|');
COPY lineitem FROM '/home/postgres/tpch_01_2025_08_16/lineitem.csv' WITH (FORMAT csv, DELIMITER '|');

