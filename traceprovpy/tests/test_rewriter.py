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

    @classmethod
    def traceprov_get_graph(cls):
        tuples = cls.run_simple_query("select * from traceprov_json_graph()")
        assert len(tuples) == 1
        main_tuple = json.loads(tuples[0][0])
        return (main_tuple["graphs"], main_tuple["context"])

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
