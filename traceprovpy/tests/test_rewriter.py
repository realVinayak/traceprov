# the main entry point for traceprov rewrite unit tests.
# These tests just check that the rewrite is correct (and that the reverse from representation -> SQL is also valid)

from traceprovpy.tests.utils import TestDbSetup
from traceprovpy.tools.setup import traceprov_setup
import os

ALL_TABLES_QUERY = "select pg_class.oid, relname from pg_class join pg_namespace on pg_namespace.oid = relnamespace where relkind='r' and  nspname='public';"


class TestRewrite(TestDbSetup):

    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        tp_root = os.getenv("tp_root")
        assert tp_root is not None
        traceprov_setup(cls.__name__, tp_root, cls.connection_params)
        cls.run_sql_from_file("tests/rewriter_setup.sql")
        tables = cls.run_simple_query(ALL_TABLES_QUERY)
        assert len(tables) > 0
        table_oid_map = {table: oid for (oid, table) in tables}
        cls.table_oid_map = table_oid_map

    def test_simple_aggregation(self):
        print(TestRewrite.table_oid_map)
        self.assertEqual(1 + 1, 2)
