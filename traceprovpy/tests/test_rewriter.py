# the main entry point for traceprov rewrite unit tests.
# These tests just check that the rewrite is correct (and that the reverse from representation -> SQL is also valid)
# to run: export tp_root=../postgres/ && export tp_skip_build=1 && source .env && python3 -m unittest tests.test_rewriter

from typing import List, Literal, NamedTuple, Tuple
from traceprovpy.tests.utils import TestDbSetup
from traceprovpy.tools.setup import (
    traceprov_make_query,
    traceprov_reinit_state,
    traceprov_setup,
)
import os
import json
from enum import Enum, auto
from sql_formatter.core import format_sql

ALL_TABLES_QUERY = "select pg_class.oid, relname from pg_class join pg_namespace on pg_namespace.oid = relnamespace where relkind='r' and  nspname='public';"


class TestRewrite(TestDbSetup):

    def simple_aggregation_no_group_by(table_name):
        return f"select count(*) from {table_name}"

    def simple_aggregation_group_by(table_name):
        return f"select count(*), var from {table_name} group by var"

    def simple_aggregation_cross_join(*tables):
        from_clause = ",".join(tables)
        return f"select count(*) from {from_clause}"

    @classmethod
    def nest_query(cls, query, num_levels=4):
        if num_levels == 0:
            return query
        return (
            f"select * from ({cls.nest_query(query, num_levels-1)}) level_{num_levels}"
        )

    def alias_query(self, query):
        self._alias_counter += 1
        return f"({query}) as ref_{self._alias_counter}"

    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        tp_root = os.getenv("tp_root")
        assert tp_root is not None
        duckdb_path = os.getenv("TRACEPROV_TEST_DUCKDB_PATH")
        assert duckdb_path is not None
        setup_response = traceprov_setup(
            cls.__name__,
            tp_root,
            cls.connection_params,
            sd_lib_path=duckdb_path,
            sd_include_path=duckdb_path,
        )
        cls.run_sql_from_file("tests/rewriter_setup.sql")
        tables = cls.run_simple_query(ALL_TABLES_QUERY)
        assert len(tables) > 0
        table_oid_map = {table: oid for (oid, table) in tables}
        cls.table_oid_map = table_oid_map
        load_query = f"LOAD '{setup_response['traceprov_rewriter_path']}'"
        print(f"LOAD QUERY: {repr(load_query)}")
        cls.run_simple_query(load_query, check_response=False)
        print(table_oid_map)

    def setUp(self):
        super().setUp()
        self._alias_counter = 0
        for table_idx in range(1, 9):
            table_name = f"table_count_2_{table_idx}_bigint"
            setattr(self, f"table_name_{table_idx}", table_name)
            setattr(
                self, f"table_oid_{table_idx}", self.__class__.table_oid_map[table_name]
            )
        TestRewrite.run_simple_query("select reinit_state();", True)

    def assertStrEqual(self, left_str: str, right_str: str):
        # print(left_str)
        return self.assertEqual(format_sql(left_str), format_sql(right_str))

    @classmethod
    def get_json_result(cls, query: str):
        tuples = cls.run_simple_query(query)
        assert len(tuples) == 1
        main_tuple = json.loads(tuples[0][0])
        return main_tuple

    @classmethod
    def traceprov_get_graph(cls):
        main_tuple = cls.get_json_result("select * from traceprov_json_graph()")
        res = (main_tuple["graphs"], main_tuple["context"])
        print(res[0])
        print(res[1])
        return res

    # gets the spec.
    # also massages it to make things easier.
    @classmethod
    def traceprov_get_spec(cls):
        spec_result = cls.get_json_result(
            "select * from traceprov_get_generic_derivation_spec(false);"
        )
        return {res["idx"]: res for res in spec_result["elements"]}

    def _assert_simple_context(self, context: dict[str, list[dict]]):
        self.assertEqual(context["setPaddingMap"], [])
        self.assertEqual(context["setGraphMap"], [])
        self.assertEqual(context["sublinks"], [])

    def test_simple_aggregation_no_group_by_1_pk(self):
        query = TestRewrite.simple_aggregation_no_group_by(self.table_name_1)
        tp_query = traceprov_make_query(query)
        print("traceprov query: ", tp_query)
        print(TestRewrite.run_simple_query(tp_query))
        graphs, context = TestRewrite.traceprov_get_graph()
        self._assert_simple_context(context)
        self.assertEqual(len(graphs), 1)
        # print(graphs)
        expected_entry = [
            {
                "graphType": "LOG",
                "headNumber": 2,
                "entries": [
                    "[TraceProvEntry (kind: TP_ENTRY_KIND_POINTER, relid: 0, resno: 0, attrNumber: 0, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]"
                ],
                "children": [
                    {
                        "graphType": "PURE_AGGREGATE",
                        "headNumber": 1,
                        "entries": [
                            f"[TraceProvEntry (kind: TP_ENTRY_KIND_BASE_RELATION, relid: {self.table_oid_1}, resno: 1, attrNumber: 1, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]"
                        ],
                        "children": [{"graphType": "NULL"}],
                    }
                ],
            }
        ]
        self.assertEqual(graphs, expected_entry)

    def test_simple_aggregation_no_group_by_1_pk_nested_levels(self):
        query = TestRewrite.nest_query(
            TestRewrite.simple_aggregation_no_group_by(self.table_name_1)
        )
        tp_query = traceprov_make_query(query)
        print("traceprov query: ", tp_query)
        print(TestRewrite.run_simple_query(tp_query))
        graphs, context = TestRewrite.traceprov_get_graph()
        self._assert_simple_context(context)
        self.assertEqual(len(graphs), 1)
        # print(graphs)

        expected_entry = [
            {
                "graphType": "LOG",
                "headNumber": 2,
                "entries": [
                    "[TraceProvEntry (kind: TP_ENTRY_KIND_POINTER, relid: 0, resno: 0, attrNumber: 0, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]"
                ],
                "children": [
                    {
                        "graphType": "PURE_AGGREGATE",
                        "headNumber": 1,
                        "entries": [
                            f"[TraceProvEntry (kind: TP_ENTRY_KIND_BASE_RELATION, relid: {self.table_oid_1}, resno: 1, attrNumber: 1, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]"
                        ],
                        "children": [{"graphType": "NULL"}],
                    }
                ],
            }
        ]

        self.assertEqual(graphs, expected_entry)

    def test_simple_aggregation_group_by_1_pk(self):
        query = TestRewrite.simple_aggregation_group_by(self.table_name_1)
        tp_query = traceprov_make_query(query)
        print("traceprov query: ", tp_query)
        print(TestRewrite.run_simple_query(tp_query))
        graphs, context = TestRewrite.traceprov_get_graph()
        self._assert_simple_context(context)
        self.assertEqual(len(graphs), 1)
        # print(graphs)

        expected_entry = [
            {
                "graphType": "LOG",
                "headNumber": 2,
                "entries": [
                    "[TraceProvEntry (kind: TP_ENTRY_KIND_POINTER, relid: 0, resno: 0, attrNumber: 0, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]"
                ],
                "children": [
                    {
                        "graphType": "AGGREGATE",
                        "headNumber": 1,
                        "entries": [
                            f"[TraceProvEntry (kind: TP_ENTRY_KIND_BASE_RELATION, relid: {self.table_oid_1}, resno: 1, attrNumber: 1, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]"
                        ],
                        "children": [{"graphType": "NULL"}],
                    }
                ],
            }
        ]

        self.assertEqual(graphs, expected_entry)

    def test_simple_aggregation_group_by_1_pk_nested_levels(self):
        query = TestRewrite.nest_query(
            TestRewrite.simple_aggregation_group_by(self.table_name_1)
        )
        tp_query = traceprov_make_query(query)
        print("traceprov query: ", tp_query)
        print(TestRewrite.run_simple_query(tp_query))
        graphs, context = TestRewrite.traceprov_get_graph()
        self._assert_simple_context(context)
        self.assertEqual(len(graphs), 1)

        # print(graphs)

        expected_entry = [
            {
                "graphType": "LOG",
                "headNumber": 2,
                "entries": [
                    "[TraceProvEntry (kind: TP_ENTRY_KIND_POINTER, relid: 0, resno: 0, attrNumber: 0, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]"
                ],
                "children": [
                    {
                        "graphType": "AGGREGATE",
                        "headNumber": 1,
                        "entries": [
                            f"[TraceProvEntry (kind: TP_ENTRY_KIND_BASE_RELATION, relid: {self.table_oid_1}, resno: 1, attrNumber: 1, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]"
                        ],
                        "children": [{"graphType": "NULL"}],
                    }
                ],
            }
        ]

        self.assertEqual(graphs, expected_entry)

    def test_simple_aggregation_no_group_by_cross_join(self):

        query = TestRewrite.simple_aggregation_cross_join(
            self.table_name_1, self.table_name_2, self.table_name_3, self.table_name_4
        )
        tp_query = traceprov_make_query(query)
        print("traceprov query: ", tp_query)
        print(TestRewrite.run_simple_query(tp_query))
        graphs, context = TestRewrite.traceprov_get_graph()
        self._assert_simple_context(context)

        # print(graphs)

        expected_entries = [
            {
                "graphType": "LOG",
                "headNumber": 2,
                "entries": [
                    "[TraceProvEntry (kind: TP_ENTRY_KIND_POINTER, relid: 0, resno: 0, attrNumber: 0, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]"
                ],
                "children": [
                    {
                        "graphType": "PURE_AGGREGATE",
                        "headNumber": 1,
                        "entries": [
                            f"[TraceProvEntry (kind: TP_ENTRY_KIND_BASE_RELATION, relid: {self.table_oid_1}, resno: 1, attrNumber: 1, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]",
                            f"[TraceProvEntry (kind: TP_ENTRY_KIND_BASE_RELATION, relid: {self.table_oid_2}, resno: 1, attrNumber: 1, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]",
                            f"[TraceProvEntry (kind: TP_ENTRY_KIND_BASE_RELATION, relid: {self.table_oid_3}, resno: 1, attrNumber: 1, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]",
                            f"[TraceProvEntry (kind: TP_ENTRY_KIND_BASE_RELATION, relid: {self.table_oid_4}, resno: 1, attrNumber: 1, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]",
                        ],
                        "children": [
                            {"graphType": "NULL"},
                            {"graphType": "NULL"},
                            {"graphType": "NULL"},
                            {"graphType": "NULL"},
                        ],
                    }
                ],
            }
        ]

        self.assertEqual(expected_entries, graphs)

    def test_simple_aggregation_no_group_by_cross_joins_nested(self):

        query_1 = TestRewrite.nest_query(f"select var from {self.table_name_1}", 5)
        query_2 = TestRewrite.nest_query(
            f"select var, id, var from {self.table_name_2}", 5
        )
        query_3 = TestRewrite.nest_query(
            f"select id, var, var, id + 1, var + 5, id + 90 from {self.table_name_3}", 3
        )
        query_4 = TestRewrite.nest_query(
            f"select id, var, var, id from {self.table_name_4}", 6
        )
        query_5 = TestRewrite.nest_query(f"select * from {self.table_name_5}")
        count_query = TestRewrite.simple_aggregation_cross_join(
            self.alias_query(query_1),
            self.alias_query(query_2),
            self.alias_query(query_1),
            self.alias_query(query_2),
            self.alias_query(query_3),
            self.alias_query(query_4),
            self.alias_query(query_5),
        )
        query = TestRewrite.nest_query(count_query)
        tp_query = traceprov_make_query(query)
        print("traceprov query: ", tp_query)
        print(TestRewrite.run_simple_query(tp_query))
        graphs, context = TestRewrite.traceprov_get_graph()
        self._assert_simple_context(context)

        # print(graphs)

        expected = [
            {
                "graphType": "LOG",
                "headNumber": 2,
                "entries": [
                    "[TraceProvEntry (kind: TP_ENTRY_KIND_POINTER, relid: 0, resno: 0, attrNumber: 0, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]"
                ],
                "children": [
                    {
                        "graphType": "PURE_AGGREGATE",
                        "headNumber": 1,
                        "entries": [
                            f"[TraceProvEntry (kind: TP_ENTRY_KIND_BASE_RELATION, relid: {self.table_oid_1}, resno: 2, attrNumber: 1, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]",
                            f"[TraceProvEntry (kind: TP_ENTRY_KIND_BASE_RELATION, relid: {self.table_oid_2}, resno: 4, attrNumber: 1, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]",
                            f"[TraceProvEntry (kind: TP_ENTRY_KIND_BASE_RELATION, relid: {self.table_oid_1}, resno: 2, attrNumber: 1, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]",
                            f"[TraceProvEntry (kind: TP_ENTRY_KIND_BASE_RELATION, relid: {self.table_oid_2}, resno: 4, attrNumber: 1, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]",
                            f"[TraceProvEntry (kind: TP_ENTRY_KIND_BASE_RELATION, relid: {self.table_oid_3}, resno: 7, attrNumber: 1, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]",
                            f"[TraceProvEntry (kind: TP_ENTRY_KIND_BASE_RELATION, relid: {self.table_oid_4}, resno: 5, attrNumber: 1, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]",
                            f"[TraceProvEntry (kind: TP_ENTRY_KIND_BASE_RELATION, relid: {self.table_oid_5}, resno: 3, attrNumber: 1, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]",
                        ],
                        "children": [
                            {"graphType": "NULL"},
                            {"graphType": "NULL"},
                            {"graphType": "NULL"},
                            {"graphType": "NULL"},
                            {"graphType": "NULL"},
                            {"graphType": "NULL"},
                            {"graphType": "NULL"},
                        ],
                    }
                ],
            }
        ]

        self.assertEqual(graphs, expected)

    def test_nested_aggregation_group_by_cross_joins(self):
        sub_query_1 = self.alias_query(
            TestRewrite.simple_aggregation_group_by(self.table_name_1)
        )
        sub_query_2 = self.alias_query(
            TestRewrite.simple_aggregation_group_by(self.table_name_2)
        )
        sub_query_3 = self.alias_query(
            TestRewrite.simple_aggregation_group_by(self.table_name_3)
        )
        sub_query_4 = self.alias_query(
            TestRewrite.simple_aggregation_group_by(self.table_name_4)
        )
        query = TestRewrite.simple_aggregation_cross_join(
            sub_query_1, sub_query_2, sub_query_3, sub_query_4
        )
        query = TestRewrite.nest_query(query)
        tp_query = traceprov_make_query(query)
        print("traceprov query: ", tp_query)
        print(TestRewrite.run_simple_query(tp_query))
        graphs, context = TestRewrite.traceprov_get_graph()
        self._assert_simple_context(context)
        # print(graphs)

        expected = [
            {
                "graphType": "LOG",
                "headNumber": 6,
                "entries": [
                    "[TraceProvEntry (kind: TP_ENTRY_KIND_POINTER, relid: 0, resno: 0, attrNumber: 0, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]"
                ],
                "children": [
                    {
                        "graphType": "PURE_AGGREGATE",
                        "headNumber": 5,
                        "entries": [
                            "[TraceProvEntry (kind: TP_ENTRY_KIND_POINTER, relid: 0, resno: 0, attrNumber: 0, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]",
                            "[TraceProvEntry (kind: TP_ENTRY_KIND_POINTER, relid: 0, resno: 0, attrNumber: 0, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]",
                            "[TraceProvEntry (kind: TP_ENTRY_KIND_POINTER, relid: 0, resno: 0, attrNumber: 0, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]",
                            "[TraceProvEntry (kind: TP_ENTRY_KIND_POINTER, relid: 0, resno: 0, attrNumber: 0, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]",
                        ],
                        "children": [
                            {
                                "graphType": "AGGREGATE",
                                "headNumber": 1,
                                "entries": [
                                    f"[TraceProvEntry (kind: TP_ENTRY_KIND_BASE_RELATION, relid: {self.table_oid_1}, resno: 1, attrNumber: 1, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]"
                                ],
                                "children": [{"graphType": "NULL"}],
                            },
                            {
                                "graphType": "AGGREGATE",
                                "headNumber": 2,
                                "entries": [
                                    f"[TraceProvEntry (kind: TP_ENTRY_KIND_BASE_RELATION, relid: {self.table_oid_2}, resno: 1, attrNumber: 1, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]"
                                ],
                                "children": [{"graphType": "NULL"}],
                            },
                            {
                                "graphType": "AGGREGATE",
                                "headNumber": 3,
                                "entries": [
                                    f"[TraceProvEntry (kind: TP_ENTRY_KIND_BASE_RELATION, relid: {self.table_oid_3}, resno: 1, attrNumber: 1, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]"
                                ],
                                "children": [{"graphType": "NULL"}],
                            },
                            {
                                "graphType": "AGGREGATE",
                                "headNumber": 4,
                                "entries": [
                                    f"[TraceProvEntry (kind: TP_ENTRY_KIND_BASE_RELATION, relid: {self.table_oid_4}, resno: 1, attrNumber: 1, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]"
                                ],
                                "children": [{"graphType": "NULL"}],
                            },
                        ],
                    }
                ],
            }
        ]
        self.assertEqual(expected, graphs)

    def test_uncorrelated_subquery_single_level_simple_log(self):
        query_1 = f"select * from {self.table_name_1} where (select count(*) from {self.table_name_2} where id != 0) > 0"
        tp_query = traceprov_make_query(query_1)
        print(TestRewrite.run_simple_query(tp_query))
        graphs, context = TestRewrite.traceprov_get_graph()
        expected_main_graph = [
            {
                "graphType": "LOG",
                "headNumber": 3,
                "entries": [
                    f"[TraceProvEntry (kind: TP_ENTRY_KIND_BASE_RELATION, relid: {self.table_oid_1}, resno: 3, attrNumber: 1, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]"
                ],
                "children": [{"graphType": "NULL"}],
            }
        ]
        self.assertEqual(graphs, expected_main_graph)

        expected_sublinks = [
            {
                "graphType": "LOG",
                "headNumber": 2,
                "entries": [
                    "[TraceProvEntry (kind: TP_ENTRY_KIND_POINTER, relid: 0, resno: 0, attrNumber: 0, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]"
                ],
                "children": [
                    {
                        "graphType": "PURE_AGGREGATE",
                        "headNumber": 1,
                        "entries": [
                            f"[TraceProvEntry (kind: TP_ENTRY_KIND_BASE_RELATION, relid: {self.table_oid_2}, resno: 1, attrNumber: 1, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]"
                        ],
                        "children": [{"graphType": "NULL"}],
                    }
                ],
            }
        ]
        self.assertEqual(context["sublinks"], expected_sublinks)

        spec = TestRewrite.traceprov_get_spec()

        expected_query_1 = """
        SELECT tp_table_1.column_0,
            tp_table_2.column_1
        FROM (
                SELECT top_level_tp_table_1.column_0::bigint
                FROM traceprov_read_worker_layer(1::int, 2::int) AS top_level_tp_table_1
            ) as tp_table_1(column_0)
            JOIN (
                SELECT intermediate_join_tp_table_2.column_0::bigint,
                    intermediate_join_tp_table_2.column_1::bigint
                FROM traceprov_read_worker_layer(1::int, 1::int) AS intermediate_join_tp_table_2
            ) as tp_table_2(column_0, column_1) ON (tp_table_1.column_0 = tp_table_2.column_0)
		"""

        expected_query_3 = """
		SELECT top_level_tp_table_0.column_0::bigint
		FROM traceprov_read_worker_layer(1::int, 3::int) AS top_level_tp_table_0
		"""

        self.assertStrEqual(spec[1]["sql"], expected_query_1)
        self.assertStrEqual(spec[3]["sql"], expected_query_3)

    def test_uncorrelated_subquery_single_level_agg_log(self):
        query_1 = f"""
            select count(*)
            from {self.table_name_1}
            where (
                            select count(*)
                            from {self.table_name_2}
                            where id != 0
                    ) > 0
            group by var;
        """
        tp_query = traceprov_make_query(query_1)
        print(TestRewrite.run_simple_query(tp_query))
        graphs, context = TestRewrite.traceprov_get_graph()

        expected_main_graph = [
            {
                "graphType": "LOG",
                "headNumber": 4,
                "entries": [
                    "[TraceProvEntry (kind: TP_ENTRY_KIND_POINTER, relid: 0, resno: 0, attrNumber: 0, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]"
                ],
                "children": [
                    {
                        "graphType": "AGGREGATE",
                        "headNumber": 3,
                        "entries": [
                            f"[TraceProvEntry (kind: TP_ENTRY_KIND_BASE_RELATION, relid: {self.table_oid_1}, resno: 1, attrNumber: 1, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]"
                        ],
                        "children": [{"graphType": "NULL"}],
                    }
                ],
            }
        ]
        self.assertEqual(graphs, expected_main_graph)

        expected_sublinks = [
            {
                "graphType": "LOG",
                "headNumber": 2,
                "entries": [
                    "[TraceProvEntry (kind: TP_ENTRY_KIND_POINTER, relid: 0, resno: 0, attrNumber: 0, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]"
                ],
                "children": [
                    {
                        "graphType": "PURE_AGGREGATE",
                        "headNumber": 1,
                        "entries": [
                            f"[TraceProvEntry (kind: TP_ENTRY_KIND_BASE_RELATION, relid: {self.table_oid_2}, resno: 1, attrNumber: 1, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]"
                        ],
                        "children": [{"graphType": "NULL"}],
                    }
                ],
            }
        ]

        self.assertEqual(context["sublinks"], expected_sublinks)
        spec = TestRewrite.traceprov_get_spec()

        expected_query_1 = """
        SELECT tp_table_1.column_0,
            tp_table_2.column_1
        FROM (
                SELECT top_level_tp_table_2.column_0::bigint
                FROM traceprov_read_worker_layer(1::int, 2::int) AS top_level_tp_table_2
            ) as tp_table_1(column_0)
            JOIN (
                SELECT intermediate_join_tp_table_3.column_0::bigint,
                    intermediate_join_tp_table_3.column_1::bigint
                FROM traceprov_read_worker_layer(1::int, 1::int) AS intermediate_join_tp_table_3
            ) as tp_table_2(column_0, column_1) ON (tp_table_1.column_0 = tp_table_2.column_0)
            """

        expected_query_3 = """
        SELECT tp_table_1.column_0,
            tp_table_2.column_1
        FROM (
                SELECT top_level_tp_table_0.column_0::bigint
                FROM traceprov_read_worker_layer(1::int, 4::int) AS top_level_tp_table_0
            ) as tp_table_1(column_0)
            JOIN (
                SELECT intermediate_join_tp_table_1.column_0::bigint,
                    intermediate_join_tp_table_1.column_1::bigint
                FROM traceprov_read_worker_layer(1::int, 3::int) AS intermediate_join_tp_table_1
            ) as tp_table_2(column_0, column_1) ON (tp_table_1.column_0 = tp_table_2.column_0)
        """

        self.assertStrEqual(spec[1]["sql"], expected_query_1)
        self.assertStrEqual(spec[3]["sql"], expected_query_3)

    def test_correlated_subquery_single_level_simple_log(self):
        query_1 = f"""
        select *
        from {self.table_name_1} as a
        where (
                        select count(*)
                        from {self.table_name_2} as b
                        where b.id != 0
                                and (a.id + b.id) > 0
                ) > 0
        """
        tp_query_1 = traceprov_make_query(query_1)
        TestRewrite.run_simple_query(tp_query_1)
        graphs, context = TestRewrite.traceprov_get_graph()

        expected_main_graphs = [
            {
                "graphType": "LOG",
                "headNumber": 3,
                "entries": [
                    f"[TraceProvEntry (kind: TP_ENTRY_KIND_BASE_RELATION, relid: {self.table_oid_1}, resno: 3, attrNumber: 1, setNumber: 0, sublinks: [(2, 0)], window: [], is_ptr_for_window: 0, is_nullable: 0)]"
                ],
                "children": [{"graphType": "NULL"}],
            }
        ]
        expected_sublinks = [
            {
                "graphType": "LOG",
                "headNumber": 2,
                "entries": [
                    "[TraceProvEntry (kind: TP_ENTRY_CORRELATION_ATTR, relid: 0, resno: 0, attrNumber: 0, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]",
                    "[TraceProvEntry (kind: TP_ENTRY_KIND_POINTER, relid: 0, resno: 0, attrNumber: 0, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]",
                ],
                "children": [
                    {"graphType": "NULL"},
                    {
                        "graphType": "PURE_AGGREGATE",
                        "headNumber": 1,
                        "entries": [
                            f"[TraceProvEntry (kind: TP_ENTRY_KIND_BASE_RELATION, relid: {self.table_oid_2}, resno: 1, attrNumber: 1, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]"
                        ],
                        "children": [{"graphType": "NULL"}],
                    },
                ],
            }
        ]

        self.assertEqual(graphs, expected_main_graphs)
        self.assertEqual(expected_sublinks, context["sublinks"])

        spec = TestRewrite.traceprov_get_spec()

        expected_query_3 = """
            SELECT top_level_tp_table_0.column_0::bigint 
            FROM traceprov_read_worker_layer(1::int, 3::int) AS top_level_tp_table_0
        """

        expected_query_1 = """
        SELECT tp_table_1.column_0,
            tp_table_1.column_1,
            tp_table_1.column_2,
            tp_table_4.column_1
        FROM (
                SELECT tp_table_2.column_0,
                    tp_table_3.column_0,
                    tp_table_3.column_1
                FROM (
                        SELECT top_level_tp_table_0.column_0::bigint
                        FROM traceprov_read_worker_layer(1::int, 3::int) AS top_level_tp_table_0
                    ) as tp_table_2(column_0)
                    JOIN (
                        SELECT log_read_to_append_tp_table_1.column_0::bigint,
                            log_read_to_append_tp_table_1.column_1::bigint
                        FROM traceprov_read_worker_layer(1::int, 2::int) AS log_read_to_append_tp_table_1
                    ) as tp_table_3(column_0, column_1) ON (tp_table_2.column_0 = tp_table_3.column_0)
            ) as tp_table_1(column_0, column_1, column_2)
            JOIN (
                SELECT intermediate_join_tp_table_2.column_0::bigint,
                    intermediate_join_tp_table_2.column_1::bigint
                FROM traceprov_read_worker_layer(1::int, 1::int) AS intermediate_join_tp_table_2
            ) as tp_table_4(column_0, column_1) ON (tp_table_1.column_2 = tp_table_4.column_0)
        """

        self.assertStrEqual(spec[1]["sql"], expected_query_1)
        self.assertStrEqual(spec[3]["sql"], expected_query_3)

    def test_correlated_subquery_single_level_agg_log(self):
        query_1 = f"""
        select count(*), var
        from {self.table_name_1} as a
        where (
                        select count(*)
                        from {self.table_name_2} as b
                        where b.id != 0
                                and (a.id + b.id) > 0
                ) > 0
        group by var
        """
        tp_query_1 = traceprov_make_query(query_1)
        TestRewrite.run_simple_query(tp_query_1)
        graphs, context = TestRewrite.traceprov_get_graph()

        expected_main_graph = [
            {
                "graphType": "LOG",
                "headNumber": 4,
                "entries": [
                    "[TraceProvEntry (kind: TP_ENTRY_KIND_POINTER, relid: 0, resno: 0, attrNumber: 0, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]"
                ],
                "children": [
                    {
                        "graphType": "AGGREGATE",
                        "headNumber": 3,
                        "entries": [
                            f"[TraceProvEntry (kind: TP_ENTRY_KIND_BASE_RELATION, relid: {self.table_oid_1}, resno: 1, attrNumber: 1, setNumber: 0, sublinks: [(2, 0)], window: [], is_ptr_for_window: 0, is_nullable: 0)]"
                        ],
                        "children": [{"graphType": "NULL"}],
                    }
                ],
            }
        ]

        expected_sublinks = [
            {
                "graphType": "LOG",
                "headNumber": 2,
                "entries": [
                    "[TraceProvEntry (kind: TP_ENTRY_CORRELATION_ATTR, relid: 0, resno: 0, attrNumber: 0, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]",
                    "[TraceProvEntry (kind: TP_ENTRY_KIND_POINTER, relid: 0, resno: 0, attrNumber: 0, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]",
                ],
                "children": [
                    {"graphType": "NULL"},
                    {
                        "graphType": "PURE_AGGREGATE",
                        "headNumber": 1,
                        "entries": [
                            f"[TraceProvEntry (kind: TP_ENTRY_KIND_BASE_RELATION, relid: {self.table_oid_2}, resno: 1, attrNumber: 1, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]"
                        ],
                        "children": [{"graphType": "NULL"}],
                    },
                ],
            }
        ]

        self.assertEqual(graphs, expected_main_graph)
        self.assertEqual(expected_sublinks, context["sublinks"])

        spec = TestRewrite.traceprov_get_spec()

        expected_query_1 = """
            SELECT tp_table_1.column_0,
                tp_table_1.column_1,
                tp_table_1.column_2,
                tp_table_1.column_3,
                tp_table_6.column_1
            FROM (
                    SELECT tp_table_2.column_0,
                        tp_table_2.column_1,
                        tp_table_5.column_0,
                        tp_table_5.column_1
                    FROM (
                            SELECT tp_table_3.column_0,
                                tp_table_4.column_1
                            FROM (
                                    SELECT top_level_tp_table_0.column_0::bigint
                                    FROM traceprov_read_worker_layer(1::int, 4::int) AS top_level_tp_table_0
                                ) as tp_table_3(column_0)
                                JOIN (
                                    SELECT intermediate_join_tp_table_1.column_0::bigint,
                                        intermediate_join_tp_table_1.column_1::bigint
                                    FROM traceprov_read_worker_layer(1::int, 3::int) AS intermediate_join_tp_table_1
                                ) as tp_table_4(column_0, column_1) ON (tp_table_3.column_0 = tp_table_4.column_0)
                        ) as tp_table_2(column_0, column_1)
                        JOIN (
                            SELECT log_read_to_append_tp_table_2.column_0::bigint,
                                log_read_to_append_tp_table_2.column_1::bigint
                            FROM traceprov_read_worker_layer(1::int, 2::int) AS log_read_to_append_tp_table_2
                        ) as tp_table_5(column_0, column_1) ON (tp_table_2.column_1 = tp_table_5.column_0)
                ) as tp_table_1(column_0, column_1, column_2, column_3)
                JOIN (
                    SELECT intermediate_join_tp_table_3.column_0::bigint,
                        intermediate_join_tp_table_3.column_1::bigint
                    FROM traceprov_read_worker_layer(1::int, 1::int) AS intermediate_join_tp_table_3
                ) as tp_table_6(column_0, column_1) ON (tp_table_1.column_3 = tp_table_6.column_0)
        """

        expected_query_3 = """
        SELECT tp_table_1.column_0,
            tp_table_2.column_1
        FROM (
                SELECT top_level_tp_table_0.column_0::bigint
                FROM traceprov_read_worker_layer(1::int, 4::int) AS top_level_tp_table_0
            ) as tp_table_1(column_0)
            JOIN (
                SELECT intermediate_join_tp_table_1.column_0::bigint,
                    intermediate_join_tp_table_1.column_1::bigint
                FROM traceprov_read_worker_layer(1::int, 3::int) AS intermediate_join_tp_table_1
            ) as tp_table_2(column_0, column_1) ON (tp_table_1.column_0 = tp_table_2.column_0)
        """

        self.assertStrEqual(spec[1]["sql"], expected_query_1)
        self.assertStrEqual(spec[3]["sql"], expected_query_3)

    def _correlated_two_level_chain_results(self):
        expected_query_3 = """
        SELECT tp_table_1.column_0,
            tp_table_1.column_1,
            tp_table_1.column_2,
            tp_table_1.column_3,
            tp_table_6.column_1
        FROM (
                SELECT tp_table_2.column_0,
                    tp_table_2.column_1,
                    tp_table_5.column_0,
                    tp_table_5.column_1
                FROM (
                        SELECT tp_table_3.column_0,
                            tp_table_4.column_1
                        FROM (
                                SELECT top_level_tp_table_0.column_0::bigint
                                FROM traceprov_read_worker_layer(1::int, 6::int) AS top_level_tp_table_0
                            ) as tp_table_3(column_0)
                            JOIN (
                                SELECT intermediate_join_tp_table_1.column_0::bigint,
                                    intermediate_join_tp_table_1.column_1::bigint
                                FROM traceprov_read_worker_layer(1::int, 5::int) AS intermediate_join_tp_table_1
                            ) as tp_table_4(column_0, column_1) ON (tp_table_3.column_0 = tp_table_4.column_0)
                    ) as tp_table_2(column_0, column_1)
                    JOIN (
                        SELECT log_read_to_append_tp_table_2.column_0::bigint,
                            log_read_to_append_tp_table_2.column_1::bigint
                        FROM traceprov_read_worker_layer(1::int, 4::int) AS log_read_to_append_tp_table_2
                    ) as tp_table_5(column_0, column_1) ON (tp_table_2.column_1 = tp_table_5.column_0)
            ) as tp_table_1(column_0, column_1, column_2, column_3)
            JOIN (
                SELECT intermediate_join_tp_table_3.column_0::bigint,
                    intermediate_join_tp_table_3.column_1::bigint
                FROM traceprov_read_worker_layer(1::int, 3::int) AS intermediate_join_tp_table_3
            ) as tp_table_6(column_0, column_1) ON (tp_table_1.column_3 = tp_table_6.column_0)
        """

        expected_query_5 = """
        SELECT tp_table_1.column_0,
            tp_table_2.column_1
        FROM (
                SELECT top_level_tp_table_0.column_0::bigint
                FROM traceprov_read_worker_layer(1::int, 6::int) AS top_level_tp_table_0
            ) as tp_table_1(column_0)
            JOIN (
                SELECT intermediate_join_tp_table_1.column_0::bigint,
                    intermediate_join_tp_table_1.column_1::bigint
                FROM traceprov_read_worker_layer(1::int, 5::int) AS intermediate_join_tp_table_1
            ) as tp_table_2(column_0, column_1) ON (tp_table_1.column_0 = tp_table_2.column_0)
        """

        return dict(query_3=expected_query_3, query_5=expected_query_5)

    def test_correlated_subquery_two_level_chain(self):
        query = f"""
        select count(*)
        from {self.table_name_1} as a
        where (
                        select count(*)
                        from {self.table_name_2} as b
                        where b.id != 0
                                and (a.id + b.id) > 0
                                and (
                                        select count(*)
                                        from {self.table_name_3} as c
                                        where c.id != b.id
                                ) > 0
                ) > 0
        """
        tp_query_1 = traceprov_make_query(query)
        TestRewrite.run_simple_query(tp_query_1)
        graphs, context = TestRewrite.traceprov_get_graph()

        expected_graph = [
            {
                "graphType": "LOG",
                "headNumber": 6,
                "entries": [
                    "[TraceProvEntry (kind: TP_ENTRY_KIND_POINTER, relid: 0, resno: 0, attrNumber: 0, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]"
                ],
                "children": [
                    {
                        "graphType": "PURE_AGGREGATE",
                        "headNumber": 5,
                        "entries": [
                            f"[TraceProvEntry (kind: TP_ENTRY_KIND_BASE_RELATION, relid: {self.table_oid_1}, resno: 1, attrNumber: 1, setNumber: 0, sublinks: [(4, 0)], window: [], is_ptr_for_window: 0, is_nullable: 0)]"
                        ],
                        "children": [{"graphType": "NULL"}],
                    }
                ],
            }
        ]

        expected_sublinks = [
            {
                "graphType": "LOG",
                "headNumber": 2,
                "entries": [
                    "[TraceProvEntry (kind: TP_ENTRY_CORRELATION_ATTR, relid: 0, resno: 0, attrNumber: 0, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]",
                    "[TraceProvEntry (kind: TP_ENTRY_KIND_POINTER, relid: 0, resno: 0, attrNumber: 0, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]",
                ],
                "children": [
                    {"graphType": "NULL"},
                    {
                        "graphType": "PURE_AGGREGATE",
                        "headNumber": 1,
                        "entries": [
                            f"[TraceProvEntry (kind: TP_ENTRY_KIND_BASE_RELATION, relid: {self.table_oid_3}, resno: 1, attrNumber: 1, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]"
                        ],
                        "children": [{"graphType": "NULL"}],
                    },
                ],
            },
            {
                "graphType": "LOG",
                "headNumber": 4,
                "entries": [
                    "[TraceProvEntry (kind: TP_ENTRY_CORRELATION_ATTR, relid: 0, resno: 0, attrNumber: 0, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]",
                    "[TraceProvEntry (kind: TP_ENTRY_KIND_POINTER, relid: 0, resno: 0, attrNumber: 0, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]",
                ],
                "children": [
                    {"graphType": "NULL"},
                    {
                        "graphType": "PURE_AGGREGATE",
                        "headNumber": 3,
                        "entries": [
                            f"[TraceProvEntry (kind: TP_ENTRY_KIND_BASE_RELATION, relid: {self.table_oid_2}, resno: 1, attrNumber: 1, setNumber: 0, sublinks: [(2, 0)], window: [], is_ptr_for_window: 0, is_nullable: 0)]"
                        ],
                        "children": [{"graphType": "NULL"}],
                    },
                ],
            },
        ]

        self.assertEqual(graphs, expected_graph)
        self.assertEqual(expected_sublinks, context["sublinks"])

        spec = TestRewrite.traceprov_get_spec()

        expected_query_1 = """
        SELECT tp_table_1.column_0,
            tp_table_1.column_1,
            tp_table_1.column_2,
            tp_table_1.column_3,
            tp_table_1.column_4,
            tp_table_1.column_5,
            tp_table_1.column_6,
            tp_table_10.column_1
        FROM (
                SELECT tp_table_2.column_0,
                    tp_table_2.column_1,
                    tp_table_2.column_2,
                    tp_table_2.column_3,
                    tp_table_2.column_4,
                    tp_table_9.column_0,
                    tp_table_9.column_1
                FROM (
                        SELECT tp_table_3.column_0,
                            tp_table_3.column_1,
                            tp_table_3.column_2,
                            tp_table_3.column_3,
                            tp_table_8.column_1
                        FROM (
                                SELECT tp_table_4.column_0,
                                    tp_table_4.column_1,
                                    tp_table_7.column_0,
                                    tp_table_7.column_1
                                FROM (
                                        SELECT tp_table_5.column_0,
                                            tp_table_6.column_1
                                        FROM (
                                                SELECT top_level_tp_table_0.column_0::bigint
                                                FROM traceprov_read_worker_layer(1::int, 6::int) AS top_level_tp_table_0
                                            ) as tp_table_5(column_0)
                                            JOIN (
                                                SELECT intermediate_join_tp_table_1.column_0::bigint,
                                                    intermediate_join_tp_table_1.column_1::bigint
                                                FROM traceprov_read_worker_layer(1::int, 5::int) AS intermediate_join_tp_table_1
                                            ) as tp_table_6(column_0, column_1) ON (tp_table_5.column_0 = tp_table_6.column_0)
                                    ) as tp_table_4(column_0, column_1)
                                    JOIN (
                                        SELECT log_read_to_append_tp_table_2.column_0::bigint,
                                            log_read_to_append_tp_table_2.column_1::bigint
                                        FROM traceprov_read_worker_layer(1::int, 4::int) AS log_read_to_append_tp_table_2
                                    ) as tp_table_7(column_0, column_1) ON (tp_table_4.column_1 = tp_table_7.column_0)
                            ) as tp_table_3(column_0, column_1, column_2, column_3)
                            JOIN (
                                SELECT intermediate_join_tp_table_3.column_0::bigint,
                                    intermediate_join_tp_table_3.column_1::bigint
                                FROM traceprov_read_worker_layer(1::int, 3::int) AS intermediate_join_tp_table_3
                            ) as tp_table_8(column_0, column_1) ON (tp_table_3.column_3 = tp_table_8.column_0)
                    ) as tp_table_2(column_0, column_1, column_2, column_3, column_4)
                    JOIN (
                        SELECT log_read_to_append_tp_table_4.column_0::bigint,
                            log_read_to_append_tp_table_4.column_1::bigint
                        FROM traceprov_read_worker_layer(1::int, 2::int) AS log_read_to_append_tp_table_4
                    ) as tp_table_9(column_0, column_1) ON (tp_table_2.column_4 = tp_table_9.column_0)
            ) as tp_table_1(
                column_0,
                column_1,
                column_2,
                column_3,
                column_4,
                column_5,
                column_6
            )
            JOIN (
                SELECT intermediate_join_tp_table_5.column_0::bigint,
                    intermediate_join_tp_table_5.column_1::bigint
                FROM traceprov_read_worker_layer(1::int, 1::int) AS intermediate_join_tp_table_5
            ) as tp_table_10(column_0, column_1) ON (tp_table_1.column_6 = tp_table_10.column_0)
            """

        expected_query = self._correlated_two_level_chain_results()
        self.assertStrEqual(spec[1]["sql"], expected_query_1)
        self.assertStrEqual(spec[3]["sql"], expected_query["query_3"])
        self.assertStrEqual(spec[5]["sql"], expected_query["query_5"])

    def test_correlated_subquery_two_level_chain_join_removal(self):
        query = f"""
        select count(*)
        from {self.table_name_1} as a
        where (
                        select count(*)
                        from {self.table_name_2} as b
                        where b.id != 0
                                and (a.id + b.id) > 0
                                and (
                                        select count(*)
                                        from {self.table_name_3} as c
                                        where c.id != (b.id + a.id)
                                ) > 0
                ) > 0
        """
        tp_query_1 = traceprov_make_query(query)
        TestRewrite.run_simple_query(tp_query_1)
        graphs, context = TestRewrite.traceprov_get_graph()

        expected_main_graph = [
            {
                "graphType": "LOG",
                "headNumber": 6,
                "entries": [
                    "[TraceProvEntry (kind: TP_ENTRY_KIND_POINTER, relid: 0, resno: 0, attrNumber: 0, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]"
                ],
                "children": [
                    {
                        "graphType": "PURE_AGGREGATE",
                        "headNumber": 5,
                        "entries": [
                            f"[TraceProvEntry (kind: TP_ENTRY_KIND_BASE_RELATION, relid: {self.table_oid_1}, resno: 1, attrNumber: 1, setNumber: 0, sublinks: [(2, 1),(4, 0)], window: [], is_ptr_for_window: 0, is_nullable: 0)]"
                        ],
                        "children": [{"graphType": "NULL"}],
                    }
                ],
            }
        ]

        expected_sublinks = [
            {
                "graphType": "LOG",
                "headNumber": 2,
                "entries": [
                    "[TraceProvEntry (kind: TP_ENTRY_CORRELATION_ATTR, relid: 0, resno: 0, attrNumber: 0, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]",
                    "[TraceProvEntry (kind: TP_ENTRY_CORRELATION_ATTR, relid: 0, resno: 0, attrNumber: 0, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]",
                    "[TraceProvEntry (kind: TP_ENTRY_KIND_POINTER, relid: 0, resno: 0, attrNumber: 0, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]",
                ],
                "children": [
                    {"graphType": "NULL"},
                    {"graphType": "NULL"},
                    {
                        "graphType": "PURE_AGGREGATE",
                        "headNumber": 1,
                        "entries": [
                            f"[TraceProvEntry (kind: TP_ENTRY_KIND_BASE_RELATION, relid: {self.table_oid_3}, resno: 1, attrNumber: 1, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]"
                        ],
                        "children": [{"graphType": "NULL"}],
                    },
                ],
            },
            {
                "graphType": "LOG",
                "headNumber": 4,
                "entries": [
                    "[TraceProvEntry (kind: TP_ENTRY_CORRELATION_ATTR, relid: 0, resno: 0, attrNumber: 0, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]",
                    "[TraceProvEntry (kind: TP_ENTRY_KIND_POINTER, relid: 0, resno: 0, attrNumber: 0, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]",
                ],
                "children": [
                    {"graphType": "NULL"},
                    {
                        "graphType": "PURE_AGGREGATE",
                        "headNumber": 3,
                        "entries": [
                            f"[TraceProvEntry (kind: TP_ENTRY_KIND_BASE_RELATION, relid: {self.table_oid_2}, resno: 1, attrNumber: 1, setNumber: 0, sublinks: [(2, 0)], window: [], is_ptr_for_window: 0, is_nullable: 0)]"
                        ],
                        "children": [{"graphType": "NULL"}],
                    },
                ],
            },
        ]

        self.assertEqual(graphs, expected_main_graph)
        self.assertEqual(expected_sublinks, context["sublinks"])

        spec = TestRewrite.traceprov_get_spec()

        expected_query_1 = """
        SELECT tp_table_1.column_0,
            tp_table_1.column_1,
            tp_table_1.column_2,
            tp_table_1.column_3,
            tp_table_1.column_4,
            tp_table_1.column_5,
            tp_table_1.column_6,
            tp_table_1.column_7,
            tp_table_10.column_1
        FROM (
                SELECT tp_table_2.column_0,
                    tp_table_2.column_1,
                    tp_table_2.column_2,
                    tp_table_2.column_3,
                    tp_table_2.column_4,
                    tp_table_9.column_0,
                    tp_table_9.column_1,
                    tp_table_9.column_2
                FROM (
                        SELECT tp_table_3.column_0,
                            tp_table_3.column_1,
                            tp_table_3.column_2,
                            tp_table_3.column_3,
                            tp_table_8.column_1
                        FROM (
                                SELECT tp_table_4.column_0,
                                    tp_table_4.column_1,
                                    tp_table_7.column_0,
                                    tp_table_7.column_1
                                FROM (
                                        SELECT tp_table_5.column_0,
                                            tp_table_6.column_1
                                        FROM (
                                                SELECT top_level_tp_table_0.column_0::bigint
                                                FROM traceprov_read_worker_layer(1::int, 6::int) AS top_level_tp_table_0
                                            ) as tp_table_5(column_0)
                                            JOIN (
                                                SELECT intermediate_join_tp_table_1.column_0::bigint,
                                                    intermediate_join_tp_table_1.column_1::bigint
                                                FROM traceprov_read_worker_layer(1::int, 5::int) AS intermediate_join_tp_table_1
                                            ) as tp_table_6(column_0, column_1) ON (tp_table_5.column_0 = tp_table_6.column_0)
                                    ) as tp_table_4(column_0, column_1)
                                    JOIN (
                                        SELECT log_read_to_append_tp_table_2.column_0::bigint,
                                            log_read_to_append_tp_table_2.column_1::bigint
                                        FROM traceprov_read_worker_layer(1::int, 4::int) AS log_read_to_append_tp_table_2
                                    ) as tp_table_7(column_0, column_1) ON (tp_table_4.column_1 = tp_table_7.column_0)
                            ) as tp_table_3(column_0, column_1, column_2, column_3)
                            JOIN (
                                SELECT intermediate_join_tp_table_3.column_0::bigint,
                                    intermediate_join_tp_table_3.column_1::bigint
                                FROM traceprov_read_worker_layer(1::int, 3::int) AS intermediate_join_tp_table_3
                            ) as tp_table_8(column_0, column_1) ON (tp_table_3.column_3 = tp_table_8.column_0)
                    ) as tp_table_2(column_0, column_1, column_2, column_3, column_4)
                    JOIN (
                        SELECT log_read_to_append_tp_table_4.column_0::bigint,
                            log_read_to_append_tp_table_4.column_1::bigint,
                            log_read_to_append_tp_table_4.column_2::bigint
                        FROM traceprov_read_worker_layer(1::int, 2::int) AS log_read_to_append_tp_table_4
                    ) as tp_table_9(column_0, column_1, column_2) ON (
                        tp_table_2.column_4 = tp_table_9.column_0
                        AND tp_table_2.column_1 = tp_table_9.column_1
                    )
            ) as tp_table_1(
                column_0,
                column_1,
                column_2,
                column_3,
                column_4,
                column_5,
                column_6,
                column_7
            )
            JOIN (
                SELECT intermediate_join_tp_table_5.column_0::bigint,
                    intermediate_join_tp_table_5.column_1::bigint
                FROM traceprov_read_worker_layer(1::int, 1::int) AS intermediate_join_tp_table_5
            ) as tp_table_10(column_0, column_1) ON (tp_table_1.column_7 = tp_table_10.column_0)
        """

        expected_query = self._correlated_two_level_chain_results()

        self.assertStrEqual(spec[1]["sql"], expected_query_1)
        self.assertStrEqual(spec[3]["sql"], expected_query["query_3"])
        self.assertStrEqual(spec[5]["sql"], expected_query["query_5"])

    def test_same_sink_subquery(self):
        query = f"""
        select *
        from {self.table_name_1} as a
        where (
                        select count(*)
                        from {self.table_name_2} as b
                        where (
                                        select count(*)
                                        from {self.table_name_3} as c
                                        where c.id != (a.id + b.id)
                                ) > 0
                ) > 0
        """
        tp_query = traceprov_make_query(query)
        TestRewrite.run_simple_query(tp_query)
        graphs, context = TestRewrite.traceprov_get_graph()

        expected_main_graph = [
            {
                "graphType": "LOG",
                "headNumber": 5,
                "entries": [
                    f"[TraceProvEntry (kind: TP_ENTRY_KIND_BASE_RELATION, relid: {self.table_oid_1}, resno: 3, attrNumber: 1, setNumber: 0, sublinks: [(2, 1)], window: [], is_ptr_for_window: 0, is_nullable: 0)]"
                ],
                "children": [{"graphType": "NULL"}],
            }
        ]

        expected_sublinks = [
            {
                "graphType": "LOG",
                "headNumber": 2,
                "entries": [
                    "[TraceProvEntry (kind: TP_ENTRY_CORRELATION_ATTR, relid: 0, resno: 0, attrNumber: 0, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]",
                    "[TraceProvEntry (kind: TP_ENTRY_CORRELATION_ATTR, relid: 0, resno: 0, attrNumber: 0, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]",
                    "[TraceProvEntry (kind: TP_ENTRY_KIND_POINTER, relid: 0, resno: 0, attrNumber: 0, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]",
                ],
                "children": [
                    {"graphType": "NULL"},
                    {"graphType": "NULL"},
                    {
                        "graphType": "PURE_AGGREGATE",
                        "headNumber": 1,
                        "entries": [
                            f"[TraceProvEntry (kind: TP_ENTRY_KIND_BASE_RELATION, relid: {self.table_oid_3}, resno: 1, attrNumber: 1, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]"
                        ],
                        "children": [{"graphType": "NULL"}],
                    },
                ],
            },
            {
                "graphType": "LOG",
                "headNumber": 4,
                "entries": [
                    "[TraceProvEntry (kind: TP_ENTRY_KIND_POINTER, relid: 0, resno: 0, attrNumber: 0, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]"
                ],
                "children": [
                    {
                        "graphType": "PURE_AGGREGATE",
                        "headNumber": 3,
                        "entries": [
                            f"[TraceProvEntry (kind: TP_ENTRY_KIND_BASE_RELATION, relid: {self.table_oid_2}, resno: 1, attrNumber: 1, setNumber: 0, sublinks: [(2, 0)], window: [], is_ptr_for_window: 0, is_nullable: 0)]"
                        ],
                        "children": [{"graphType": "NULL"}],
                    }
                ],
            },
        ]

        self.assertEqual(graphs, expected_main_graph)
        self.assertEqual(expected_sublinks, context["sublinks"])

        spec = TestRewrite.traceprov_get_spec()

    def test_same_sink_subquery_correlated(self):
        query = f"""
        select *
        from { self.table_name_1 } as a
        where (
                        select count(*)
                        from { self.table_name_2 } as b
                        where (
                                        (
                                                select count(*)
                                                from { self.table_name_3 } as c
                                                where (
                                                                select count(*)
                                                                from { self.table_name_4 } as d
                                                                where d.id != b.id + c.id
                                                        ) > 0
                                                        and (c.id != a.id)
                                        ) > 0
                                )
                                and b.id != a.id
                ) > 0
        """
        tp_query = traceprov_make_query(query)
        TestRewrite.run_simple_query(tp_query)
        graphs, context = TestRewrite.traceprov_get_graph()

        expected_main_graph = [
            {
                "graphType": "LOG",
                "headNumber": 7,
                "entries": [
                    "[TraceProvEntry (kind: TP_ENTRY_KIND_BASE_RELATION, relid: 410202, resno: 3, attrNumber: 1, setNumber: 0, sublinks: [(4, 0),(6, 0)], window: [], is_ptr_for_window: 0, is_nullable: 0)]"
                ],
                "children": [{"graphType": "NULL"}],
            }
        ]

        sublinks = [
            {
                "graphType": "LOG",
                "headNumber": 2,
                "entries": [
                    "[TraceProvEntry (kind: TP_ENTRY_CORRELATION_ATTR, relid: 0, resno: 0, attrNumber: 0, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]",
                    "[TraceProvEntry (kind: TP_ENTRY_CORRELATION_ATTR, relid: 0, resno: 0, attrNumber: 0, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]",
                    "[TraceProvEntry (kind: TP_ENTRY_KIND_POINTER, relid: 0, resno: 0, attrNumber: 0, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]",
                ],
                "children": [
                    {"graphType": "NULL"},
                    {"graphType": "NULL"},
                    {
                        "graphType": "PURE_AGGREGATE",
                        "headNumber": 1,
                        "entries": [
                            "[TraceProvEntry (kind: TP_ENTRY_KIND_BASE_RELATION, relid: 410217, resno: 1, attrNumber: 1, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]"
                        ],
                        "children": [{"graphType": "NULL"}],
                    },
                ],
            },
            {
                "graphType": "LOG",
                "headNumber": 4,
                "entries": [
                    "[TraceProvEntry (kind: TP_ENTRY_CORRELATION_ATTR, relid: 0, resno: 0, attrNumber: 0, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]",
                    "[TraceProvEntry (kind: TP_ENTRY_KIND_POINTER, relid: 0, resno: 0, attrNumber: 0, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]",
                ],
                "children": [
                    {"graphType": "NULL"},
                    {
                        "graphType": "PURE_AGGREGATE",
                        "headNumber": 3,
                        "entries": [
                            "[TraceProvEntry (kind: TP_ENTRY_KIND_BASE_RELATION, relid: 410212, resno: 1, attrNumber: 1, setNumber: 0, sublinks: [(2, 0)], window: [], is_ptr_for_window: 0, is_nullable: 0)]"
                        ],
                        "children": [{"graphType": "NULL"}],
                    },
                ],
            },
            {
                "graphType": "LOG",
                "headNumber": 6,
                "entries": [
                    "[TraceProvEntry (kind: TP_ENTRY_CORRELATION_ATTR, relid: 0, resno: 0, attrNumber: 0, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]",
                    "[TraceProvEntry (kind: TP_ENTRY_KIND_POINTER, relid: 0, resno: 0, attrNumber: 0, setNumber: 0, sublinks: [], window: [], is_ptr_for_window: 0, is_nullable: 0)]",
                ],
                "children": [
                    {"graphType": "NULL"},
                    {
                        "graphType": "PURE_AGGREGATE",
                        "headNumber": 5,
                        "entries": [
                            "[TraceProvEntry (kind: TP_ENTRY_KIND_BASE_RELATION, relid: 410207, resno: 1, attrNumber: 1, setNumber: 0, sublinks: [(2, 1)], window: [], is_ptr_for_window: 0, is_nullable: 0)]"
                        ],
                        "children": [{"graphType": "NULL"}],
                    },
                ],
            },
        ]

        spec = TestRewrite.traceprov_get_spec()
        print(spec)
