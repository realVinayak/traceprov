from functools import reduce
from typing import NamedTuple

from traceprovpy.tools.run_with_timeout import ReplaceSelectivity

# it is much more nicer to do it this way.
class SkewValue(NamedTuple):
    first: int
    second: int

    def serialize(self):
        return f"{self.first}_{self.second}"
    
    @staticmethod
    def deserialize(in_str: str):
        assert "_" in in_str
        split = in_str.split('_')
        assert len(split) == 2
        return SkewValue(*split)

def make_replacer(num_rows, skew_value: SkewValue):
    replacers = get_replacers(num_rows, skew_value)
    def replacer(in_sql: str):
        replaced = reduce(lambda prev, curr: curr.preprocess(prev), replacers, in_sql)
        return replaced
    return replacer

def get_replacers(num_rows, skew_value: SkewValue):
    # replacers = []
    replacers= [(ReplaceSelectivity(str(num_rows), "ROW_COUNT", is_strict=True)) ]
    if skew_value:
        skew_value_str = skew_value.serialize()
        replacers.append(ReplaceSelectivity(skew_value_str, "SKEW_VALUE", is_strict=True))
    return replacers