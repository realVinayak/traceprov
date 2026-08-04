select
    add_provenance('part');

select
    add_provenance('supplier');

select
    add_provenance('partsupp');

select
    add_provenance('customer');

select
    add_provenance('orders');

select
    add_provenance('lineitem');

select
    add_provenance('nation');

select
    add_provenance('region');

SELECT
    create_provenance_mapping('part_formula_mapping', 'part', 'provenance');

SELECT
    create_provenance_mapping(
        'supplier_formula_mapping',
        'supplier',
        'provenance'
    );

SELECT
    create_provenance_mapping(
        'partsupp_formula_mapping',
        'partsupp',
        'provenance'
    );

SELECT
    create_provenance_mapping(
        'customer_formula_mapping',
        'customer',
        'provenance'
    );

SELECT
    create_provenance_mapping(
        'orders_formula_mapping',
        'orders',
        'provenance'
    );

SELECT
    create_provenance_mapping(
        'lineitem_formula_mapping',
        'lineitem',
        'provenance'
    );

SELECT
    create_provenance_mapping(
        'nation_formula_mapping',
        'nation',
        'provenance'
    );

SELECT
    create_provenance_mapping(
        'region_formula_mapping',
        'region',
        'provenance'
    );

CREATE TABLE formula_map AS
SELECT
    *
FROM
    part_formula_mapping
UNION
SELECT
    *
FROM
    supplier_formula_mapping
UNION
SELECT
    *
FROM
    partsupp_formula_mapping
UNION
SELECT
    *
FROM
    customer_formula_mapping
UNION
SELECT
    *
FROM
    orders_formula_mapping
UNION
SELECT
    *
FROM
    lineitem_formula_mapping
UNION
SELECT
    *
FROM
    nation_formula_mapping
UNION
SELECT
    *
FROM
    region_formula_mapping;

ALTER TABLE
    formula_map
ADD
    PRIMARY KEY (provenance);