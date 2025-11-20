from unittest import TestCase
import psycopg2
import os

from traceprovpy.tools.run_with_timeout import ConnectionParams


class TestDbSetup(TestCase):
    test_db: str = "test_traceprov_db"

    @classmethod
    def run_sql_from_file(cls, root_file_path: str):
        db_cursor = cls.db_connection.cursor()
        try:
            with open(f"{os.getcwd()}/{root_file_path}") as f:
                sql_stmts = f.read()
            db_cursor.execute(sql_stmts)
            db_cursor.execute("COMMIT;")
        except:
            db_cursor.execute("ROLLBACK;")
            raise
        db_cursor.close()

    @classmethod
    def run_simple_query(cls, query, check_response=True):
        db_cursor = cls.db_connection.cursor()
        db_cursor.execute(query)
        if check_response:
            result = db_cursor.fetchall()
        else:
            result = None
        db_cursor.close()
        return result

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
        if cls.pg_user is None or cls.pg_password is None:
            raise Exception("Set TRACEPROV_TEST_USER and TRACEPROV_TEST_PASSWORD")
        host = os.getenv("TRACEPROV_TEST_HOST", "127.0.0.1")
        port = os.getenv("TRACEPROV_TEST_PORT", "5432")
        cls.pg_host = host
        cls.pg_port = port
        cls.intial_connection = initial_connection

        cursor = initial_connection.cursor()
        cursor.execute("COMMIT;")
        cursor.execute(f"DROP DATABASE IF EXISTS {cls.test_db}")
        cursor.execute(f"CREATE DATABASE {cls.test_db};")
        cursor.close()

        db_connection = psycopg2.connect(
            database=cls.test_db,
            host=cls.pg_host,
            port=cls.pg_port,
            user=cls.pg_user,
            password=cls.pg_password,
        )

        cls.db_connection = db_connection
        cls.run_sql_from_file("tests/setup.sql")
        cls.connection_params = ConnectionParams(
            host=cls.pg_host,
            port=cls.pg_port,
            user=cls.pg_user,
            password=cls.pg_password,
            database=cls.test_db,
        )

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
