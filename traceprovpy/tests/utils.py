from unittest import TestCase
import psycopg2
import os


class TestDbSetup(TestCase):
    test_db: str = "test_traceprov_db"

    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        initial_connection = psycopg2.connect(
            database="postgres",
            host="127.0.0.1",
            user=os.getenv("TRACEPROV_TEST_USER"),
            password=os.getenv("TRACEPROV_TEST_PASSWORD"),
            port="5432",
        )
        cls.pg_user = os.getenv("TRACEPROV_TEST_USER")
        cls.pg_password = os.getenv("TRACEPROV_TEST_PASSWORD")
        cls.intial_connection = initial_connection

        cursor = initial_connection.cursor()
        cursor.execute("COMMIT;")
        cursor.execute(f"DROP DATABASE IF EXISTS {cls.test_db}")
        cursor.execute(f"CREATE DATABASE {cls.test_db};")
        cursor.close()

        db_connection = psycopg2.connect(
            database=cls.test_db,
            host="127.0.0.1",
            user=os.getenv("TRACEPROV_TEST_USER"),
            password=os.getenv("TRACEPROV_TEST_PASSWORD"),
            port="5432",
        )

        cls.db_connection = db_connection
        db_cursor = db_connection.cursor()
        try:
            with open(f"{os.getcwd()}/tests/setup.sql") as f:
                sql_stmts = f.read()
            db_cursor.execute(sql_stmts)
            db_cursor.execute("COMMIT;")
        except:
            db_cursor.execute("ROLLBACK;")
            raise
        db_cursor.close()

    @classmethod
    def tearDownClass(cls):
        super().tearDownClass()
        # Need to close this before destroying.
        cls.db_connection.close()

        cursor = cls.intial_connection.cursor()
        cursor.execute(f"DROP DATABASE IF EXISTS {cls.test_db}")
        cursor.close()
        cls.intial_connection.close()

    def setUp(self):
        super().setUp()
        self.cursor = self.__class__.db_connection.cursor()

    def tearDown(self):
        super().tearDown()
        self.cursor.close()
