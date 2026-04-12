SELECT writeaggregation(
            2,
            array_agg("subquery2"."tuid")
      ) AS "tuid",
      sum(
            ("subquery2"."l_extendedprice") * (
                  (1) - ("subquery2"."l_discount")
            )
      ) AS "revenue"
FROM (
            SELECT writejoin(
                        1,
                        "RTE0"."tuid",
                        "RTE1"."tuid"
                  ) AS "tuid",
                  "RTE0"."l_extendedprice" AS "l_extendedprice",
                  "RTE0"."l_discount" AS "l_discount"
            FROM lineitem_1 AS "RTE0"(
                        "tuid",
                        "l_orderkey",
                        "l_partkey",
                        "l_suppkey",
                        "l_linenumber",
                        "l_quantity",
                        "l_extendedprice",
                        "l_discount",
                        "l_tax",
                        "l_returnflag",
                        "l_linestatus",
                        "l_shipdate",
                        "l_commitdate",
                        "l_receiptdate",
                        "l_shipinstruct",
                        "l_shipmode",
                        "l_comment"
                  ),
                  part_1 AS "RTE1"(
                        "tuid",
                        "p_partkey",
                        "p_name",
                        "p_mfgr",
                        "p_brand",
                        "p_type",
                        "p_size",
                        "p_container",
                        "p_retailprice",
                        "p_comment"
                  )
            WHERE ("RTE1"."p_partkey") = ("RTE0"."l_partkey")
                  AND ("RTE1"."p_brand") = ('Brand#12')
                  AND "RTE1"."p_container" = ANY(
                        ARRAY ['SM CASE', 
                                             'SM BOX', 
                                             'SM PACK', 
                                             'SM PKG']::bpchar []
                  )
                  AND ("RTE0"."l_quantity") >= (1)
                  AND ("RTE0"."l_quantity") <= ((1) + (10))
                  AND ("RTE1"."p_size") >= (1)
                  AND ("RTE1"."p_size") <= (5)
                  AND "RTE0"."l_shipmode" = ANY(
                        ARRAY ['AIR', 
                                                'AIR REG']::bpchar []
                  )
                  AND ("RTE0"."l_shipinstruct") = ('DELIVER IN PERSON')
                  OR ("RTE1"."p_partkey") = ("RTE0"."l_partkey")
                  AND ("RTE1"."p_brand") = ('Brand#23')
                  AND "RTE1"."p_container" = ANY(
                        ARRAY ['MED BAG', 
                                             'MED BOX', 
                                             'MED PKG', 
                                             'MED PACK']::bpchar []
                  )
                  AND ("RTE0"."l_quantity") >= (10)
                  AND ("RTE0"."l_quantity") <= ((10) + (10))
                  AND ("RTE1"."p_size") >= (1)
                  AND ("RTE1"."p_size") <= (10)
                  AND "RTE0"."l_shipmode" = ANY(
                        ARRAY ['AIR', 
                                                'AIR REG']::bpchar []
                  )
                  AND ("RTE0"."l_shipinstruct") = ('DELIVER IN PERSON')
                  OR ("RTE1"."p_partkey") = ("RTE0"."l_partkey")
                  AND ("RTE1"."p_brand") = ('Brand#34')
                  AND "RTE1"."p_container" = ANY(
                        ARRAY ['LG CASE', 
                                             'LG BOX', 
                                             'LG PACK', 
                                             'LG PKG']::bpchar []
                  )
                  AND ("RTE0"."l_quantity") >= (20)
                  AND ("RTE0"."l_quantity") <= ((20) + (10))
                  AND ("RTE1"."p_size") >= (1)
                  AND ("RTE1"."p_size") <= (15)
                  AND "RTE0"."l_shipmode" = ANY(
                        ARRAY ['AIR', 
                                                'AIR REG']::bpchar []
                  )
                  AND ("RTE0"."l_shipinstruct") = ('DELIVER IN PERSON')
      ) AS "subquery2"(
            "tuid",
            "l_extendedprice",
            "l_discount"
      )
HAVING True;