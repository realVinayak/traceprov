def postgres_connection_from_cmd(parser):
    parser.add_argument("-u", "--user", required=True)
    parser.add_argument("-p", "--password", required=True)
    parser.add_argument("-H", "--host", required=False, default="127.0.0.1")
    parser.add_argument("-P", "--port", required=False, default="5432")
    parser.add_argument("-db", "--db", required=True)
