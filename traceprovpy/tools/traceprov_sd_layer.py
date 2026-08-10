EQ_LAYER_COMP = """
{q: '2', l: 3},
{q: '4', l: 2},
{q: '17', l: 3},
{q: '18', l: 3},
{q: '20', l: 5},
{q: '21', l: 2},
{q: '22', l: 3}
"""

ALL_LAYER_COMP = """
{q: NULL, l: NULL}
"""


KEY = "TRACEPROV_EQ_LAYER_COMPARISON"


def add_tp_sd_eq_layer_comp(in_sql: str):
    assert KEY in in_sql
    return in_sql.replace(KEY, EQ_LAYER_COMP)


def set_all_tp_sd_comp(in_sql):
    assert KEY in in_sql
    return in_sql.replace(KEY, ALL_LAYER_COMP)


QUERY_LAYERS = {
    "2": [3],
    "4": [2],
    "17": [3],
    "18": [3],
    "20": [5],
    "21": [2],
    "22": [3],
}


def _join_l(ls):
    return ",".join(map(str, ls))


def check():
    eq_created = ",".join(
        [
            "{" + f"q: '{q}', l: {_join_l(layers)}" + "}"
            for (q, layers) in sorted(QUERY_LAYERS.items(), key=lambda x: int(x[0]))
        ]
    )
    cleaned_main = EQ_LAYER_COMP.replace("\n", "").strip()
    assert eq_created == cleaned_main, f"Mismatch: {eq_created} != {cleaned_main}"


check()


def get_eq_queries():
    return sorted(list(QUERY_LAYERS.keys()), key=lambda x: int(x[0]))
