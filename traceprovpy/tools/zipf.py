from functools import reduce
import random

import numpy as np


def zipf_draw(card, num_groups, skew):
    probs = generate_zifpian_distribution(num_groups, skew)
    input_probs = make_cdf(probs)
    return [_draw(input_probs) for _ in range(card)]


def generate_zifpian_distribution(num_groups, skew_param):
    zeta_value = sum([1 / pow(i, skew_param) for i in range(1, num_groups + 1)])
    probs = [1 / (pow(i, skew_param) * zeta_value) for i in range(1, num_groups + 1)]
    assert np.isclose(sum(probs), 1)
    # print(sum(probs))
    return probs


def make_cdf(probs):

    def _reducer(previous, current):
        last = previous[-1]
        return [*previous, current + last]

    return reduce(_reducer, probs[1:], [probs[0]])


def _draw(input_probs):
    random_value = random.random()
    found = -1
    for idx, prob in enumerate(input_probs):
        if random_value <= prob:
            found = idx
            break
    if found == -1:
        print("truncating beyond to end")
        found = len(input_probs) - 1
    return found
