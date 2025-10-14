from traceprovpy.tests.utils import TestDbSetup


class TestSimple(TestDbSetup):

    def test_simple(self):
        self.assertEqual(1 + 1, 2)

    def test_simple_fetch(self):
        self.cursor.execute("SELECT * FROM test_table order by a, b;")
        results = self.cursor.fetchall()
        self.assertEqual(results, [(1, 1), (1, 3), (2, 5), (8, 9)])
