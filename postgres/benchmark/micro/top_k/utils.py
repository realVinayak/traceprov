from functools import reduce

from traceprovpy.tools.run_with_timeout import ReplaceSelectivity


def make_replacer(num_rows, top_k):
    replacers = get_replacers(num_rows, top_k)
    def replacer(in_sql: str):
        replaced = reduce(lambda prev, curr: curr.preprocess(prev), replacers, in_sql)
        return replaced
    return replacer

def get_replacers(num_rows, top_k):
    # replacers = []
    replacers= [(ReplaceSelectivity(str(num_rows), "ROW_COUNT")) ]
    if top_k:
        replacers.append(ReplaceSelectivity(str(top_k), ":top_k_limit"))
    return replacers